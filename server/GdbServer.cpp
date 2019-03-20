// GDB RSP server implementation: definition

// Copyright (C) 2009, 2013, 2017  Embecosm Limited <info@embecosm.com>

// Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
// Contributor Ian Bolton <ian.bolton@embecosm.com>

// This file is part of the RISC-V GDB server

// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License for more details.

// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>
#include <cassert>

#include "GdbServer.h"
#include "embdebug/Utils.h"
#include "VContActions.h"
#include "SyscallReplyPacket.h"

using std::chrono::system_clock;
using std::cout;
using std::cerr;
using std::dec;
using std::endl;
using std::hex;
using std::localtime;
using std::ostringstream;
using std::setfill;
using std::setw;
using std::string;
using std::stringstream;
using std::vector;

//! Constructor for the GDB RSP server.

//! Allocate a packet data structure and a new RSP connection. By default no
//! timeout for run/continue.

//! @param[in] rspPort      RSP port to use.
//! @param[in] _cpu         The simulated CPU
//! @param[in] _data           Data shared with the targets
//! @param[in] _killBehaviour  How to handle ctrl-C

GdbServer::GdbServer (AbstractConnection * _conn,
		      ITarget * _cpu,
                      TraceFlags *traceFlags,
		      KillBehaviour _killBehaviour) :
  cpu (_cpu),
  traceFlags (traceFlags),
  rsp (_conn),
  pkt (RSP_PKT_SIZE),
  killBehaviour (_killBehaviour),
  mExitServer (false),
  mHaveMultiProc (false),
  mStopMode (StopMode::ALL_STOP),
  mPtid (PID_DEFAULT, TID_DEFAULT),
  mNextProcess (1),
  mHandlingSyscall (false),
  mKillCoreOnExit (false),
  mCoreManager (cpu->getCpuCount ())
{
}	// GdbServer ()


//! Destructor

GdbServer::~GdbServer ()
{
}	// ~GdbServer


//! Main loop to listen for RSP requests

//! This only terminates if there was an error.

int
GdbServer::rspServer ()
{
  // Loop processing commands forever
  while (!mExitServer)
    {
      // Make sure we are still connected.
      while (!rsp->isConnected ())
	{
	  // Reconnect and stall the processor on a new connection
	  if (!rsp->rspConnect ())
	    {
	      // Serious failure. Must abort execution.
	      cerr << "*** Unable to continue: EXITING" << endl;
	      return EXIT_FAILURE;
	    }

          // Calling reset restores all cores to life.  Maybe this isn't
          // the right thing to do?  Maybe we want exited cores to stay
          // exited even over a disconnect and reconnect... but I'm
          // guessing that in most cases a disconnect and reconnect implies
          // that we're starting again with the target and would like all
          // cores to spring back to life.
          mCoreManager.reset ();
	}

      // Get a RSP client request
      rspClientRequest ();
    }

  return EXIT_SUCCESS;
}	// rspServer ()


//! Some F request packets want to know the length of the string
//! argument, so we have this simple function here to calculate that.

int
GdbServer::stringLength (uint32_t addr)
{
  uint8_t ch;
  int count = 0;
  while (1 == cpu->read (addr + count, &ch, 1))
  {
    count++;
    if (ch == 0)
      break;
  }
  return count;
}

//! We achieve a syscall on the host by sending an F request packet to
//! the GDB client. The arguments for the call will have already been
//! put into registers via its newlib/libgloss implementation.

void
GdbServer::rspSyscallRequest ()
{
  // Keep track of whether we were in the middle of a Continue or Step
  if (mHandlingSyscall)
    cerr << "Warning: There's already a syscall pending, first one lost?"
         << endl;
  mHandlingSyscall = true;

  // Get the args from the appropriate regs and send an F packet
  uint_reg_t a0, a1, a2, a7;
  // We don't need all the registers for every syscall, so we only read the
  // common subset here.
  cpu->readRegister (10, a0);
  cpu->readRegister (11, a1);
  cpu->readRegister (12, a2);
  cpu->readRegister (17, a7);

  // Work out which syscall we've got
  switch (a7) {
    case 57   : sprintf (pkt.data, "Fclose,%" PRIxREG, a0);
                break;
    case 62   : sprintf (pkt.data, "Flseek,%" PRIxREG
                         ",%" PRIxREG ",%" PRIxREG, a0, a1, a2);
                break;
    case 63   : sprintf (pkt.data, "Fread,%" PRIxREG
                         ",%" PRIxREG ",%" PRIxREG, a0, a1, a2);
                break;
    case 64   : sprintf (pkt.data, "Fwrite,%" PRIxREG
                         ",%" PRIxREG ",%" PRIxREG, a0, a1, a2);
                break;
    case 80   : sprintf (pkt.data, "Ffstat,%" PRIxREG ",%" PRIxREG,
                         a0, a1);
                break;
    case 93   :
      {
        if (traceFlags->traceExec ())
          cerr << "EXIT syscall on core " << cpu->getCurrentCpu ()
               << " halting all other cores." << endl;
        (void) cpu->halt ();
        if (mHaveMultiProc)
          sprintf (pkt.data, "W%" PRIxREG ";process:%x", a0,
                   CoreManager::coreNum2Pid (cpu->getCurrentCpu ()));
        else
          sprintf (pkt.data, "W%" PRIxREG, a0);
        /* We never get a reply from an exit syscall, so don't
           store a continuation state.  */
        mHandlingSyscall = false;
        /* Mark the core as dead.  */
        mCoreManager.killCoreNum (cpu->getCurrentCpu ());
      }
      break;
    case 169  : sprintf (pkt.data, "Fgettimeofday,%" PRIxREG
                         ",%" PRIxREG, a0, a1);
                break;
    case 1024 : sprintf (pkt.data, "Fopen,%" PRIxREG "/%x,%" PRIxREG
                         ",%" PRIxREG, a0, stringLength (a0), a1, a2);
                break;
    case 1026 : sprintf (pkt.data, "Funlink,%" PRIxREG "/%x",
                         a0, stringLength (a0));
                break;
    case 1038 : sprintf (pkt.data, "Fstat,%" PRIxREG "/%x,%"
                         PRIxREG, a0, stringLength (a0), a1);
                break;

    default   : rspReportException (TargetSignal::TRAP);
                return;
  }

  // Send the packet
  pkt.setLen (strlen (pkt.data));
  rsp->putPkt (pkt);
}


//! The F reply is sent by the GDB client to us after a syscall has been
//! handled.

void
GdbServer::rspSyscallReply ()
{
  SyscallReplyPacket p;

  // We've finished with the syscall.
  mHandlingSyscall = false;

  p.parse (pkt.data);

  if (p.valid ())
    {
      int retcode = p.retcode ();

      if (retcode != -1)
        cpu->writeRegister (10, retcode);

      if (p.hasCtrlC ())
        {
          // Due to timing between packet send and receive and interrupts
          // being delivered, it's possible that an interrupt is delivered
          // both through the syscall reply mechanism, and through the
          // standard break character through the connection.
          //
          // Notify GDB that the target has stopped with SIGINT, then check
          // the incoming packet stream for any pending break and remove
          // it.  It's important that we first tell GDB we've stopped, then
          // look for a pending break otherwise GDB could send the break
          // after we've checked, but before we've told GDB we've actually
          // stopped.
          if (traceFlags->traceExec ())
            cerr << "Break detected in gdbserver, halting all cores" << endl;
          (void) cpu->halt ();
          rspReportException (TargetSignal::INT);
          (void) rsp->haveBreak ();
          return;
        }
    }

  doCoreActions ();
}

// Implement a continue.

void
GdbServer::doCoreActions (void)
{
  // Check for a pending break from the user before resuming the machine.
  // If we only stopped in order to handle a syscall then some of the cores
  // might still be in a running state, halt everything before returning.
  if (rsp->haveBreak ())
    {
      if (traceFlags->traceExec ())
        cerr << "Break detected in gdbserver, halting all cores" << endl;
      (void) cpu->halt ();
      rspReportException (TargetSignal::INT);
      return;
    }

  if (processStopEvents ())
    return;

  // Calculate the wall-clock end time for this set of actions.  After this
  // amount of time has passed we will start to halt the machine
  // regardless, but coming to a halt always takes some non-zero time, so
  // this timeout is only ever approximate.

  mTimeout.timeStamp (cpu);

  if (!cpu->resume ())
    {
      cerr << "*** ABORT: Error while resuming target" << endl;
      abort ();
    }

  // Tell the target to resume this set of actions.
  std::vector <ITarget::ResumeRes> results;
  ITarget::WaitRes waitres;
  while ((waitres = cpu->wait (results)) == ITarget::WaitRes::TIMEOUT)
    {
      bool haveBreak;

      // Check for a break from gdb.
      haveBreak = rsp->haveBreak ();

      // Check for timeout, unless the timeout was zero
      if (haveBreak || mTimeout.timedOut (cpu))
        {
          // Force the target to stop. Ignore return value.
          TargetSignal sig;

          if (traceFlags->traceExec ())
            cerr << "Break detected in gdbserver, halting all cores" << endl;
          (void) cpu->halt ();
          sig = haveBreak ? TargetSignal::INT : TargetSignal::XCPU;
          rspReportException (sig);
          return;
        }
    }

  if (waitres == ITarget::WaitRes::ERROR)
    {
      cerr << "*** ABORT: Error returned from call to wait" << endl;
      abort ();
    }

  // The target has halted for some reason.
  if (results.size () != mCoreManager.getCpuCount ())
    {
      cerr << "*** ABORT: wait returned incorrect number of results, got "
           << dec << results.size () << " expected "
           << mCoreManager.getCpuCount () << endl;
      abort ();
    }

  for (unsigned int i = 0; i < mCoreManager.getCpuCount (); ++i)
    {
      if (mCoreManager[i].isRunning ())
        {
          if (mCoreManager[i].hasUnreportedStop ())
            {
              cerr << "*** ABORT: Core" << dec << i << " stopped, but "
                   << "already has a stop event pending" << endl;
              abort ();
            }
          mCoreManager[i].setStopReason (results[i]);
        }
    }

  if (processStopEvents ())
    return;

  cerr << "*** ABORT: Error no stop event found" << endl;
  abort ();
}

//! Extracts the next stop event that we should process by looking
//! at the current state of mCoreManager.  If an event is found then CPU
//! and RESUMERES are updated with the number of the cpu, and the reason
//! why the cpu stopped, this method then returns true.
//! If no event is found then false is returned, and CPU and RESUMERES are
//! not modified.

bool
GdbServer::getNextStopEvent (unsigned int &cpu,
                                 ITarget::ResumeRes &resumeRes)
{
  bool found_non_syscall;
  unsigned int non_syscall_cpu;
  ITarget::ResumeRes non_syscall_res;

  found_non_syscall = false;
  for (unsigned int i = 0; i < mCoreManager.getCpuCount (); ++i)
    {
      if (!mCoreManager[i].isRunning ()
          || !mCoreManager[i].hasUnreportedStop ())
        continue;

      ITarget::ResumeRes res = mCoreManager[i].stopReason ();
      if (res == ITarget::ResumeRes::NONE)
        continue;

      if (res == ITarget::ResumeRes::SYSCALL)
        {
          cpu = i;
          resumeRes = res;
          return true;
        }

      if (!found_non_syscall)
        {
          non_syscall_cpu = i;
          non_syscall_res = res;
          found_non_syscall = true;
          continue;
        }
    }

  if (found_non_syscall)
    {
      cpu = non_syscall_cpu;
      resumeRes = non_syscall_res;
      return true;
    }

  return false;
}	// getNextStopEvent ()

//! Find a stop event to report by looking at the current state of
//! mCoreManager, and handle the event by reporting it to GDB, then return
//! true.  If there is no event to process then return false.

bool
GdbServer::processStopEvents (void)
{
  unsigned int cpuNum;
  ITarget::ResumeRes res;

  if (getNextStopEvent (cpuNum, res))
    {
      mCoreManager[cpuNum].reportStopReason ();
      cpu->setCurrentCpu (cpuNum);
      switch (res)
        {
        case ITarget::ResumeRes::SYSCALL:
          // @todo this change of current cpu here is probably dangerous, after
          // we've finished processing the syscall, we should probably switch
          // back to the previously selected cpu.
          if (traceFlags->traceExec ())
            cerr << "processStopEvent: SYSCALL (core " << cpuNum << ")"
                 << endl;
          rspSyscallRequest ();
          return true;

        case ITarget::ResumeRes::INTERRUPTED:
          if (traceFlags->traceExec ())
            cerr << "processStopEvent: INTERRUPT (core " << cpuNum << ")"
                 << endl;
          rspReportException ();
          return true;

        case ITarget::ResumeRes::STEPPED:
          if (traceFlags->traceExec ())
            cerr << "processStopEvent: STEPPED (core " << cpuNum << ")"
                 << endl;
          rspReportException (TargetSignal::TRAP);
          return true;

        case ITarget::ResumeRes::LOCKSTEP:
          if (traceFlags->traceExec ())
            cerr << "processStopEvent: LOCKSTEP (core " << cpuNum << ")"
                 << endl;
          rspReportException (TargetSignal::USR1);
          return true;

        default:
          cerr << "*** ABORT: Unknown stop event type " << res << endl;
          abort ();
        }
    }

  return false;
}	// processStopEvent ()

//! Deal with a request from the GDB client session

//! In general, apart from the simplest requests, this function replies on
//! other functions to implement the functionality.

//! @note It is the responsibility of the recipient to delete the packet when
//!       it is finished with. It is permissible to reuse the packet for a
//!       reply.

//! @param[in] pkt  The received RSP packet

void
GdbServer::rspClientRequest ()
{
  if (!rsp->getPkt (pkt))
    {
      rsp->rspClose ();			// Comms failure
      return;
    }

  switch (pkt.data[0])
    {
    case '!':
      // Request for extended remote mode
      pkt.packStr ("OK");
      rsp->putPkt (pkt);
      return;

    case '?':
      // Return last signal ID
      {
        // @todo We need to handle asynchronous stops for non-stop mode.
        ITarget::ResumeRes stopReason =
          mCoreManager[cpu->getCurrentCpu ()].stopReason ();
        switch (stopReason)
          {
          case ITarget::ResumeRes::INTERRUPTED:
            rspReportException ();
            break;
          default:
            cerr << "*** ABORT: Unexpected stop reason: "
                 << stopReason << endl;
            abort ();
            break;
          }
      }
      return;

    case 'A':
      // Initialization of argv not supported
      cerr << "Warning: RSP 'A' packet not supported: ignored" << endl;
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;

    case 'b':
      // Setting baud rate is deprecated
      cerr << "Warning: RSP 'b' packet is deprecated and not "
	   << "supported: ignored" << endl;
      return;

    case 'B':
      // Breakpoints should be set using Z packets
      cerr << "Warning: RSP 'B' packet is deprecated (use 'Z'/'z' "
	   << "packets instead): ignored" << endl;
      return;

    case 'F':
      // Handle the syscall reply then continue
      rspSyscallReply ();
      return;

    case 'c':
    case 'C':
      // We now expect vCont instead of these packets.
      cerr << "Warning: RSP '" << pkt.data[1]
           << "' packet is not supported: ignored" << endl;
      return;

    case 'd':
      // Disable debug using a general query
      cerr << "Warning: RSP 'd' packet is deprecated (define a 'Q' "
	   << "packet instead: ignored" << endl;
      return;

    case 'D':
      // Detach GDB. Do this by closing the client. The rules say that
      // execution should continue, so unstall the processor.
      pkt.packStr("OK");
      rsp->putPkt (pkt);
      rsp->rspClose ();
      return;

    case 'g':
      rspReadAllRegs ();
      return;

    case 'G':
      rspWriteAllRegs ();
      return;

    case 'H':
      // Set the thread number of subsequent operations. Syntax is
      //
      // H <op> <ptid>
      //
      // where <op> is 'c' to set the thread for subsequent continue ops and
      // 'g' for all other ops. However 'c' is deprecated in favor of
      // supporting vCont.

      switch (pkt.data[1])
	{
	case 'c':

	  // Hc is dprecated - ignore it.

	  pkt.packStr ("");
	  rsp->putPkt (pkt);
	  return;

	case 'g':

	  // If we are told to choose any process, we choose the default
	  // process. Not clear that ALL processes is valid here.

	  if (mPtid.decode (&(pkt.data[strlen ("Hg")]))
	      && mPtid.crystalize (PID_DEFAULT, TID_DEFAULT))
            {
              // Convert process number to core number (using - 1).
              // @todo method for pid to core mapping.
              cpu->setCurrentCpu (mPtid.pid () - 1);
              pkt.packStr ("OK");
            }
	  else
	    pkt.packStr ("E01");

	  rsp->putPkt (pkt);
	  return;

	default:

	  pkt.packStr ("E02");
	  rsp->putPkt (pkt);
	  return;
	}

    case 'i':
      // Single cycle step. TODO. For now we immediately report we have hit an
      // exception.
      rspReportException ();
      return;

    case 'I':
      // Single cycle step with signal. TODO. For now we immediately report we
      // have hit an exception.
      rspReportException ();
      return;

    case 'k':
      // With multiprocess support, we expect vKill instead.
      cerr << "Warning: RSP 'k' packet is not supported: ignored" << endl;
      return;

    case 'm':
      // Read memory (symbolic)
      rspReadMem ();
      return;

    case 'M':
      // Write memory (symbolic)
      rspWriteMem ();
      return;

    case 'p':
      // Read a register
      rspReadReg ();
      return;

    case 'P':
      // Write a register
      rspWriteReg ();
      return;

    case 'q':
      // Any one of a number of query packets
      rspQuery ();
      return;

    case 'Q':
      // Any one of a number of set packets
      rspSet ();
      return;

    case 'r':
      // Reset the system. Deprecated (use 'R' instead)
      cerr << "Warning: RSP 'r' packet is deprecated (use 'R' "
 		<< "packet instead): ignored" << endl;
      return;

    case 'R':
      // Restart the program being debugged. TODO. Nothing for now.
      return;

    case 's':
    case 'S':
      // We now expect vCont instead of these packets.
      cerr << "Warning: RSP '" << pkt.data[1]
           << "' packet is not supported: ignored" << endl;
      return;

    case 't':
      // Search. This is not well defined in the manual and for now we don't
      // support it. No response is defined.
      cerr << "Warning: RSP 't' packet not supported: ignored" << endl;
      return;

    case 'T':
      // Is the thread alive. We are bare metal, so don't have a thread
      // context. The answer is always "OK".
      pkt.packStr ("OK");
      rsp->putPkt (pkt);
      return;

    case 'v':
      // Any one of a number of packets to control execution
      rspVpkt ();
      return;

    case 'X':
      // Write memory (binary)
      rspWriteMemBin ();
      return;

    case 'z':
      // Remove a breakpoint/watchpoint.
      rspRemoveMatchpoint ();
      return;

    case 'Z':
      // Insert a breakpoint/watchpoint.
      rspInsertMatchpoint ();
      return;

    default:
      // Unknown commands are ignored
      cerr << "Warning: Unknown RSP request" << pkt.data << endl;
      return;
    }
}	// rspClientRequest ()


//! Send a packet acknowledging an exception has occurred

//! @param[in] sig  The signal to send (defaults to TargetSignal::TRAP).

void
GdbServer::rspReportException (TargetSignal  sig)
{
  // Construct a signal received packet
  if (mHaveMultiProc)
    sprintf (pkt.data, "T%02xthread:p%x.1;",
             (static_cast <int> (sig) & 0xff),
             CoreManager::coreNum2Pid (cpu->getCurrentCpu ()));
  else
    sprintf (pkt.data, "S%02x", (static_cast <int> (sig) & 0xff));
  pkt.setLen (strlen (pkt.data));

  rsp->putPkt (pkt);

}	// rspReportException ()


//! Handle a RSP read all registers request

//! This means getting the value of each simulated register and packing it
//! into the packet.

//! Each byte is packed as a pair of hex digits.

void
GdbServer::rspReadAllRegs ()
{
  int  pktSize = 0;

  // The registers. GDB client expects them to be packed according to target
  // endianness.
  for (int  regNum = 0; regNum < RISCV_NUM_REGS; regNum++)
    {
      uint_reg_t val;		// Enough for even the PC
      int       byteSize;	// Size of reg in bytes

      byteSize = cpu->readRegister (regNum, val);
      Utils::regVal2Hex (val, &(pkt.data[pktSize]), byteSize,
		      true /* Little Endian */);
      pktSize += byteSize * 2;	// 2 chars per hex digit
    }

  // Finalize the packet and send it
  pkt.data[pktSize] = 0;
  pkt.setLen (pktSize);
  rsp->putPkt (pkt);

}	// rspReadAllRegs ()


//! Handle a RSP write all registers request

//! Each value is written into the simulated register.

void
GdbServer::rspWriteAllRegs ()
{
  int  pktSize = 0;

  // The registers
  for (int  regNum = 0; regNum < RISCV_NUM_REGS; regNum++)
    {
      std::size_t byteSize = sizeof (uint_reg_t);

      uint32_t val = Utils::hex2RegVal (&(pkt.data[pktSize]), byteSize,
				     true /* little endian */);
      pktSize += byteSize * 2;	// 2 chars per hex digit

      if (byteSize != cpu->writeRegister (regNum, val))
	cerr << "Warning: Size != " << byteSize << " when writing reg "
	     << regNum << "." << endl;
    }

  pkt.packStr ("OK");
  rsp->putPkt (pkt);

}	// rspWriteAllRegs ()


//! Handle a RSP read memory (symbolic) request

//! Syntax is:
//!   m<addr>,<length>:

//! The response is the bytes, lowest address first, encoded as pairs of hex
//! digits.

//! The length given is the number of bytes to be read.

void
GdbServer::rspReadMem ()
{
  uint_reg_t  addr;			// Where to read the memory
  int         len;			// Number of bytes to read
  int         off;			// Offset into the memory

  if (2 != sscanf (pkt.data, "m%" PRIxREG ",%x:", &addr, (unsigned *)&len))
    {
      cerr << "Warning: Failed to recognize RSP read memory command: "
		<< pkt.data << endl;
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Make sure we won't overflow the buffer (2 chars per byte)
  if ((len * 2) >= pkt.getBufSize())
    {
      cerr << "Warning: Memory read " << pkt.data
	   << " too large for RSP packet: truncated" << endl;
      len = (pkt.getBufSize() - 1) / 2;
    }

  // Refill the buffer with the reply
  for (off = 0; off < len; off++)
    {
      uint8_t  ch;
      uint32_t ret;

      ret = cpu->read (addr + off, &ch, 1);

      if (1 == ret)
	{
	  pkt.data[off * 2]     = Utils::hex2Char(ch >>   4);
	  pkt.data[off * 2 + 1] = Utils::hex2Char(ch &  0xf);
	}
      else
	cerr << "Warning: failed to read char" << endl;
    }

  pkt.data[off * 2] = '\0';			// End of string
  pkt.setLen (strlen (pkt.data));
  rsp->putPkt (pkt);

}	// rsp_read_mem ()


//! Handle a RSP write memory (symbolic) request

//! Syntax is:

//!   m<addr>,<length>:<data>

//! The data is the bytes, lowest address first, encoded as pairs of hex
//! digits.

//! The length given is the number of bytes to be written.

void
GdbServer::rspWriteMem ()
{
  uint32_t  addr;			// Where to write the memory
  int       len;			// Number of bytes to write

  if (2 != sscanf (pkt.data, "M%x,%x:", &addr, (unsigned *)&len))
    {
      cerr << "Warning: Failed to recognize RSP write memory "
		<< pkt.data << endl;
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Find the start of the data and check there is the amount we expect.
  char *symDat = (char *)(memchr (pkt.data, ':', pkt.getBufSize())) + 1;
  int   datLen = pkt.getLen() - (symDat - pkt.data);

  // Sanity check
  if (len * 2 != datLen)
    {
      cerr << "Warning: Write of " << len * 2 << "digits requested, but "
		<< datLen << " digits supplied: packet ignored" << endl;
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Write the bytes to memory (no check the address is OK here)
  for (int  off = 0; off < len; off++)
    {
      uint8_t  nyb1 = Utils::char2Hex (symDat[off * 2]);
      uint8_t  nyb2 = Utils::char2Hex (symDat[off * 2 + 1]);
      uint8_t  val = static_cast<unsigned int> ((nyb1 << 4) | nyb2);

      if (1 != cpu->write (addr + off, &val, 1))
	cerr << "Warning: Failed to write character" << endl;
    }

  pkt.packStr ("OK");
  rsp->putPkt (pkt);

}	// rspWriteMem ()


//! Read a single register

//! The registers follow the GDB sequence: 32 general registers, SREG, SP and
//! PC.

//! Each byte is packed as a pair of hex digits.

void
GdbServer::rspReadReg ()
{
  unsigned int  regNum;

  // Break out the fields from the data
  if (1 != sscanf (pkt.data, "p%x", &regNum))
    {
      cerr << "Warning: Failed to recognize RSP read register command: "
		<< pkt.data << endl;
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Get the relevant register. GDB client expects them to be packed according
  // to target endianness.
  uint_reg_t val;
  int byteSize;

  byteSize = cpu->readRegister (regNum, val);
  Utils::regVal2Hex (val, pkt.data, byteSize, true /* little endian */);

  pkt.setLen (strlen (pkt.data));
  rsp->putPkt (pkt);

}	// rspReadReg ()


//! Write a single register

//! The registers follow the GDB sequence for OR1K: GPR0 through GPR31, PC
//! (i.e. SPR NPC) and SR (i.e. SPR SR). The register is specified as a
//! sequence of bytes in target endian order.

//! Each byte is packed as a pair of hex digits.

void
GdbServer::rspWriteReg ()
{
  std::size_t regByteSize = sizeof (uint_reg_t);
  unsigned int regNum;
  const int valstr_len = 2 * sizeof (uint_reg_t);
  char valstr[valstr_len + 1];	// Allow for EOS

  // Break out the fields from the data
  std::ostringstream fmt_stream;
  fmt_stream << "P%x=%" << valstr_len << "s";
  string fmt = fmt_stream.str ();
  if (2 != sscanf (pkt.data, fmt.c_str (), &regNum, valstr))
    {
      cerr << "Warning: Failed to recognize RSP write register command "
	   << pkt.data << endl;
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }
  valstr[valstr_len] = '\0';

  uint_reg_t val
    = Utils::hex2RegVal (valstr, regByteSize, true /* little endian */);

  if (regByteSize != cpu->writeRegister (regNum, val))
    cerr << "Warning: Size != " << regByteSize << " when writing reg " << regNum
	 << "." << endl;

  pkt.packStr ("OK");
  rsp->putPkt (pkt);

}	// rspWriteReg ()

//! Send out a single thread info reply packet

//! Sends out information about the thread and process corresponding to
//! the process number in mNextProcess, and updates mNextProcess.  Once
//! information about all processes has been sent (by repeated calls to
//! this function) then the end marker packet will be sent instead.
void
GdbServer::rspWriteNextThreadInfo ()
{
  unsigned int coreNum;

  // When a core calls 'exit' we mark it as not-live.  When sending out
  // information about threads (cores) we only want to report on live
  // cores, so, this loop looks for the next live core.
  do
    {
      coreNum = CoreManager::pid2CoreNum (mNextProcess);
      mNextProcess++;
    }
  while (coreNum < mCoreManager.getCpuCount ()
         && (mKillCoreOnExit && !mCoreManager.isCoreLive (coreNum)));

  if (coreNum < mCoreManager.getCpuCount ())
    {
      pkt.packStr ("m");
      Ptid ptid (CoreManager::coreNum2Pid (coreNum), TID_DEFAULT);

      if (ptid.encode (&(pkt.data[pkt.getLen ()])))
        pkt.setLen (strlen (pkt.data));
      else
        pkt.packStr ("E01");
    }
  else
    pkt.packStr ("l");		// All done

  rsp->putPkt (pkt);
}

//! Handle a RSP query request

//! We deal with those we have an explicit response for and send a null
//! response to anything else, to indicate it is not supported. This makes us
//! flexible to future GDB releases with as yet undefined packets.

void
GdbServer::rspQuery ()
{
  if (0 == strcmp ("qC", pkt.data))
    {
      // Return the current thread ID (unsigned hex). A null response
      // indicates to use the previously selected thread.

      pkt.packStr ("QC");

      if (mPtid.encode (&(pkt.data[strlen ("QC")])))
	pkt.setLen (strlen (pkt.data));
      else
	pkt.packStr ("E01");

      rsp->putPkt (pkt);
    }
  else if (0 == strcmp ("qfThreadInfo", pkt.data))
    {
      // Send information about the first process.  After we send this
      // reply GDB will send additional 'qsThreadInfo' packets to get
      // information about all the other threads on the system.
      //
      // Our model of the system has one thread per process, and one
      // process per core.  The cores are number 0 -> X, while processes
      // are numbered 1 -> (X + 1).
      mNextProcess = 1;
      rspWriteNextThreadInfo ();
    }
  else if (0 == strcmp ("qsThreadInfo", pkt.data))
    {
      // Send information about the "next" thread continuing from wherever
      // the last thread info request left off.
      rspWriteNextThreadInfo ();
    }
  else if (0 == strncmp ("qL", pkt.data, strlen ("qL")))
    {
      // Deprecated and replaced by 'qfThreadInfo'
      cerr << "Warning: RSP qL deprecated: no info returned" << endl;
      pkt.packStr ("qM001");
      rsp->putPkt (pkt);
    }
  else if (0 == strncmp ("qRcmd,", pkt.data, strlen ("qRcmd,")))
    {
      // This is used to interface to commands to do "stuff"
      rspCommand ();
    }
  else if (0 == strncmp ("qSupported", pkt.data, strlen ("qSupported")))
    {
      // Report a list of the features we support. For now we just ignore any
      // supplied specific feature queries, but in the future these may be
      // supported as well. Note that the packet size allows for 'G' + all the
      // registers sent to us, or a reply to 'g' with all the registers and an
      // EOS so the buffer is a well formed string.

      vector<string> tokens;
      Utils::split (&(pkt.data[strlen ("qSupported:")]), ";", tokens);
      const char * multiProcStr = "";
      const char * xmlRegsStr   = "";

      // We can only support multiprocess and XML target descriptions if the
      // client says it supports it. Offering eitther when it is not there
      // causes some really weird behavior!

      mHaveMultiProc = false;

      for (auto it = tokens.begin (); it != tokens.end (); it++)
	if (0 == strncmp ("multiprocess+", it->c_str (),
			  strlen ("multiprocess+")))
	  {
	    mHaveMultiProc = true;
	    multiProcStr = ";multiprocess+";
	  }
	else if (0 == strncmp ("xmlRegisters=", it->c_str (),
			       strlen ("xmlRegisters=")))
	  {
	    if  (0 != strncmp ("xmlRegisters=riscv", it->c_str (),
			       strlen ("xmlRegisters=riscv")))
	      cerr << "Warning: Non RISCV XML registers offered: "
		   << "expect weird behavior." << endl;
	    xmlRegsStr = ";qXfer:features:read+";
	  }

      sprintf (pkt.data,
	       "PacketSize=%x;QNonStop+;VContSupported+%s%s",
	       pkt.getBufSize(), multiProcStr, xmlRegsStr);

      pkt.setLen (strlen (pkt.data));
      rsp->putPkt (pkt);
    }
  else if (0 == strncmp ("qSymbol:", pkt.data, strlen ("qSymbol:")))
    {
      // Offer to look up symbols. Nothing we want (for now). TODO. This just
      // ignores any replies to symbols we looked up, but we didn't want to
      // do that anyway!
      pkt.packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (0 == strncmp ("qThreadExtraInfo,", pkt.data,
			 strlen ("qThreadExtraInfo,")))
    {
      // Report that we are runnable, but the text must be hex ASCI
      // digits. For now do this by steam, reusing the original packet
      sprintf (pkt.data, "%02x%02x%02x%02x%02x%02x%02x%02x%02x",
	       'R', 'u', 'n', 'n', 'a', 'b', 'l', 'e', 0);
      pkt.setLen (strlen (pkt.data));
      rsp->putPkt (pkt);
    }
  else if (0 == strncmp ("qXfer:features:read:target.xml:", pkt.data,
			 strlen ("qXfer:features:read:target.xml:")))
    {
      // Report back our target description

      unsigned int firstChar, lastChar;

      if (2 != sscanf (pkt.data, "qXfer:features:read:target.xml:%x,%x",
		       &firstChar, &lastChar))
	{
	  pkt.packStr ("E00");
	  rsp->putPkt (pkt);
	  return;
	}

      std::ostringstream xmlStream;

      xmlStream << "<?xml version=\"1.0\"?>" << endl;
      xmlStream << "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">" << endl;
      xmlStream << "<target version=\"1.0\">" << endl;
      xmlStream << "<architecture>riscv:rv" << 8 * sizeof (uint_reg_t)
		<< "</architecture>" << endl;
      xmlStream << "</target>" <<endl;

      std::string  xmlString  = xmlStream.str ();

      // Constuct the packet

      char         pktChar;
      size_t       len;

      if (xmlString.size () > lastChar)
	{
	  // Middle packet

	  pktChar = 'm';
	  len     = lastChar - firstChar + 1;
	}
      else
	{
	  // Last packet

	  pktChar = 'l';
	  len = xmlString.size () - firstChar + 1;
	}

      sprintf (pkt.data, "%c%s", pktChar,
	       xmlString.substr (firstChar, len).c_str ());
      pkt.setLen (len + 1);			// Allow for 'm'/'l'
      rsp->putPkt (pkt);
    }
  else
    {
      // We don't support this feature
      pkt.packStr ("");
      rsp->putPkt (pkt);
    }
}	// rspQuery ()


//! Handle a RSP qRcmd request

//! The actual command follows the "qRcmd," in ASCII encoded to hex

void
GdbServer::rspCommand ()
{
  char *cmd = new char[pkt.getBufSize ()];
  uint64_t   timeout;

  Utils::hex2Ascii (cmd, &(pkt.data[strlen ("qRcmd,")]));

  if (traceFlags->traceRsp())
    {
      cout << "RSP trace: qRcmd," << cmd << endl;
    }

  if (0 == strncmp ("help", cmd, strlen (cmd)))
    {
      static const char *mess [] = {
	"The following generic monitor commands are supported:\n",
	"  help\n",
	"    Produce this message\n",
	"  reset [cold | warm]\n",
	"    Reset the simulator (default warm)\n",
	"  exit\n",
	"    Exit the GDB server\n",
	"  timeout <interval>\n",
	"    Maximum time in seconds taken by continue packet\n",
	"  real-timestamp\n",
	"    Report the wallclock time in the target\n",
	"  timestamp\n",
	"    Report the current execution time in the target\n",
	"  cyclecount\n",
	"    Report cycles executed since last report and since reset\n",
	"  instrcount\n",
	"    Report instructions executed since last report and since reset\n",
	"  set debug <flag> [on|off|0|1] [<value>]\n",
	"    Set debug flag in target and optional associated value\n",
	"  show debug [<flag>]\n",
	"    Show debug for one flag or all flags in target\n",
	"  echo <message>\n",
	"    Echo <message> on stdout of the gdbserver\n",
	nullptr };

      for (int i = 0; nullptr != mess[i]; i++)
	{
	  pkt.packRcmdStr (mess[i], true);
	  rsp->putPkt (pkt);
	}

      // Now get any help from the target

      stringstream  ss;
      if (cpu->command (string ("help"), ss))
	{
	  string line;

	  pkt.packRcmdStr ("The following target specific monitor commands are supported:\n",
			     true);
	  rsp->putPkt (pkt);
	  while (getline (ss, line, '\n'))
	    {
	      line.append ("\n");
	      pkt.packRcmdStr (line.c_str (), true);
	      rsp->putPkt (pkt);
	    }
	}
      else
	{
	  // No target specific help

	  pkt.packRcmdStr ("There are no target specific monitor commands\n",
			     true);
	  rsp->putPkt (pkt);
	}

      // Not silent, so acknowledge OK

      pkt.packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if ((0 == strcmp (cmd, "reset")) || (0 == strcmp (cmd, "reset warm")))
    {
      // First, bring all the cores back to life.
      mCoreManager.reset ();

      // Warm reset the CPU.  Failure to reset causes us to blow up.

      if (ITarget::ResumeRes::SUCCESS != cpu->reset (ITarget::ResetType::WARM))
	{
	  cerr << "*** ABORT *** Failed to reset: Terminating." << endl;
	  abort ();
	}

      pkt.packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (0 == strcmp (cmd, "reset cold"))
    {
      // First, bring all the cores back to life.
      mCoreManager.reset ();

      // Cold reset the CPU.  Failure to reset causes us to blow up.

      if (ITarget::ResumeRes::SUCCESS != cpu->reset (ITarget::ResetType::COLD))
	{
	  cerr << "*** ABORT *** Failed to cold reset: Terminating." << endl;
	  abort ();
	}

      pkt.packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (0 == strcmp (cmd, "exit"))
    {
      mExitServer = true;
    }
  else if ((1 == sscanf (cmd, "timeout %lu", &timeout))
	   || (1 == sscanf (cmd, "real-timeout %lu", &timeout)))
    {
      mTimeout.realTimeout (std::chrono::duration <double> (static_cast <double> (timeout)));
      pkt.packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (1 == sscanf (cmd, "cycle-timeout %lu", &timeout))
    {
      mTimeout.cycleTimeout (timeout);
      pkt.packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (0 == strcmp (cmd, "real-timestamp"))
    {
      // @todo Do this using std::put_time, which is not in pre 5.0 GCC. Not
      // thread safe.

      std::ostringstream  oss;
      time_t now_c = system_clock::to_time_t (system_clock::now ());
      struct tm * timeinfo = localtime (&(now_c));
      char buff[20];

      strftime (buff, 20, "%F %T", timeinfo);
      oss << buff << endl;
      pkt.packHexstr (oss.str ().c_str ());
      rsp->putPkt (pkt);

      // Not silent, so acknowledge OK

      pkt.packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (0 == strcmp (cmd, "timestamp"))
    {
      std::ostringstream  oss;
      oss << cpu->timeStamp () << endl;
      pkt.packHexstr (oss.str ().c_str ());
      rsp->putPkt (pkt);

      // Not silent, so acknowledge OK

      pkt.packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (0 == strcmp (cmd, "cyclecount"))
    {
      std::ostringstream  oss;
      oss << cpu->getCycleCount () << endl;
      pkt.packHexstr (oss.str ().c_str ());
      rsp->putPkt (pkt);

      // Not silent, so acknowledge OK

      pkt.packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (0 == strcmp (cmd, "instrcount"))
    {
      std::ostringstream  oss;
      oss << cpu->getInstrCount () << endl;
      pkt.packHexstr (oss.str ().c_str ());
      rsp->putPkt (pkt);

      // Not silent, so acknowledge OK

      pkt.packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (0 == strncmp (cmd, "echo", 4))
    {
      const char *tmp = cmd + 4;
      while (*tmp != '\0' && isspace (*tmp))
	++tmp;
      cerr << std::flush;
      cout << tmp << std::endl << std::flush;
      pkt.packStr ("OK");
      rsp->putPkt (pkt);
    }
  // Insert any new generic commands here.
  // Don't forget to document them.

  else if (0 == strncmp (cmd, "set ", strlen ("set ")))
    {
      int i;

      for (i =  strlen ("set ") ; isspace (cmd[i]) ; i++)
	;

      rspSetCommand (cmd + i);
    }
  else if (0 == strncmp (cmd, "show ", strlen ("show ")))
    {
      int i;

      for (i =  strlen ("show ") ; isspace (cmd[i]) ; i++)
	;

      rspShowCommand (cmd + i);
    }
  else
    {
      // Fallback is to pass the command to the target.

      ostringstream  oss;

      if (cpu->command (string (cmd), oss))
	{
	  pkt.packRcmdStr (oss.str ().c_str (), true);
	  rsp->putPkt (pkt);

	  // Not silent, so acknowledge OK

	  pkt.packStr ("OK");
	  rsp->putPkt (pkt);
	}
      else
	{
	  // Command failed

	  pkt.packStr ("E01");
	  rsp->putPkt (pkt);
	}
    }

  delete [] cmd;

}	// rspCommand ()


//! Handle a RSP qRcmd request for set

//! The main rspCommand function has decoded the argument string and
//! stripped off "set" and any spaces separating it from the rest.

//! Any unrecognized command is passed to the target to process.

//! @param[in] cmd  The RSP set command string (excluding "set ")

void
GdbServer::rspSetCommand (const char* cmd)
{
  vector<string> tokens;
  Utils::split (cmd, " ", tokens);
  int numTok = tokens.size ();

  // Look for any options we can handle.

  if ((2 <= numTok) && (numTok <= 4) && (string ("debug") == tokens[0]))
    {
      // Three flavors:
      // - monitor set debug <flag>
      // - monitor set debug <flag> 1|0|on|off|true|false
      // - monitor set debug <flag> 1|0|on|off|true|false <value>

      const char * flagName = tokens[1].c_str ();

      // Valid flag?

      if (!traceFlags->isFlag (flagName))
 	{
	  // Not a valid flag

 	  pkt.packStr ("E01");
 	  rsp->putPkt (pkt);
 	  return;
 	}

      bool flagState;

      if (2 == numTok)
	flagState = true;
      else
	{
	  // Valid state?

	  if ((0 == strcasecmp (tokens[2].c_str (), "0"))
	      || (0 == strcasecmp (tokens[2].c_str (), "off"))
	      || (0 == strcasecmp (tokens[2].c_str (), "false")))
	    flagState = false;
	  else if ((0 == strcasecmp (tokens[2].c_str (), "1"))
		   || (0 == strcasecmp (tokens[2].c_str (), "on"))
		   || (0 == strcasecmp (tokens[2].c_str (), "true")))
	    flagState = true;
	  else
	    {
	      // Not a valid level

	      pkt.packStr ("E02");
	      rsp->putPkt (pkt);
	      return;
	    }
	}

      // Do we have a value associated?

      if (4 == numTok)
	traceFlags->flag (flagName, flagState, tokens[3].c_str (),
                          traceFlags->isNumericFlag(flagName));
      else
	traceFlags->flagState (flagName, flagState);

      pkt.packStr ("OK");
      rsp->putPkt (pkt);
      return;
    }
  else if (string ("kill-core-on-exit") == tokens[0])
    {
      // Valid state?

      if ((0 == strcasecmp (tokens[1].c_str (), "0"))
          || (0 == strcasecmp (tokens[1].c_str (), "off"))
          || (0 == strcasecmp (tokens[1].c_str (), "false")))
        mKillCoreOnExit = false;
      else if ((0 == strcasecmp (tokens[1].c_str (), "1"))
               || (0 == strcasecmp (tokens[1].c_str (), "on"))
               || (0 == strcasecmp (tokens[1].c_str (), "true")))
        mKillCoreOnExit = true;
      else
        {
          // Not a valid level
          pkt.packStr ("E02");
          rsp->putPkt (pkt);
          return;
        }

      pkt.packStr ("OK");
      rsp->putPkt (pkt);
      return;
    }
  else
    {
      // Not handled here, try the target

      ostringstream  oss;
      string fullCmd = string ("set ") + string (cmd);

      if (cpu->command (string (fullCmd), oss))
	{
	  pkt.packRcmdStr (oss.str ().c_str (), true);
	  rsp->putPkt (pkt);

	  // Not silent, so acknowledge OK

	  pkt.packStr ("OK");
	  rsp->putPkt (pkt);
	}
      else
	{
	  // Command failed

	  pkt.packStr ("E04");
	  rsp->putPkt (pkt);
	}
    }
}	// RspSetCommand ()


//! Handle a RSP qRcmd request for show

//! The main rspCommand function has decoded the argument string and
//! stripped off "show" and any spaces separating it from the rest.

//! Any unrecognized command is passed to the target to process.

//! @param[in] cmd  The RSP command string (excluding "show ")

void
GdbServer::rspShowCommand (const char* cmd)
{
  vector<string> tokens;
  Utils::split (cmd, " ", tokens);
  int numTok = tokens.size ();

  if ((numTok == 1) && (string ("debug") == tokens[0]))
    {
      // monitor show debug

      pkt.packRcmdStr (traceFlags->dump ().c_str (), true);
      rsp->putPkt (pkt);
      pkt.packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if ((numTok == 2) && (string ("debug") == tokens[0]))
    {
      // monitor show debug <flag>

      ostringstream  oss;
      const char * flagName = tokens[1].c_str ();

      // Valid flag?

      if (!traceFlags->isFlag (flagName))
	{
	  // Not a valid flag

	  pkt.packStr ("E01");
	  rsp->putPkt (pkt);
	  return;
	}

      oss << flagName << ": "
	  << (traceFlags->flagState (flagName) ? "ON" : "OFF");

      oss << " (associated val = \"" << traceFlags->flagVal (flagName)
          << "\")";

      oss << endl;

      pkt.packRcmdStr (oss.str ().c_str (), true);
      rsp->putPkt (pkt);
      pkt.packStr ("OK");
      rsp->putPkt (pkt);
    }
  else
    {
      // Not handled here, try the target

      ostringstream  oss;
      string fullCmd = string ("show ") + string (cmd);

      if (cpu->command (string (fullCmd), oss))
	{
	  pkt.packRcmdStr (oss.str ().c_str (), true);
	  rsp->putPkt (pkt);

	  // Not silent, so acknowledge OK

	  pkt.packStr ("OK");
	  rsp->putPkt (pkt);
	}
      else
	{
	  // Command failed

	  pkt.packStr ("E04");
	  rsp->putPkt (pkt);
	}
    }
}	// rspShowCommand ()


//! Handle a RSP set request.

void
GdbServer::rspSet ()
{
  if (0 == strncmp ("QNonStop:", pkt.data, strlen ("QNonStop:")))
    {
      switch (pkt.data [strlen ("QNonStop:")])
        {
        case '0': mStopMode = StopMode::ALL_STOP; break;
        case '1': mStopMode = StopMode::NON_STOP; break;

        default:
          pkt.packStr ("E01");
          rsp->putPkt (pkt);
          return;
        }

      pkt.packStr ("OK");
      rsp->putPkt (pkt);
      return;
    }

  pkt.packStr ("");
  rsp->putPkt (pkt);

}	// rspSet ()


//! Handle a 'vCont:' packet.  The actual list of things to do is after the
//! 'vCont:' in the packet buffer.
//
//! This packet will be used in preference to the older 'c', 'C', 's', and
//! 'S' packets when GDB support is available.

void
GdbServer::rspVCont ()
{
  vector <ITarget::ResumeType> coreActions;
  VContActions actions (pkt.data);
  if (!actions.valid ())
    {
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  for (unsigned int i = 0; i < mCoreManager.getCpuCount (); ++i)
    {
      ITarget::ResumeType resType;
      char action = actions.getCoreAction (CoreManager::coreNum2Pid (i));

      switch (action)
        {
        case '\0':
          resType = ITarget::ResumeType::NONE;
          break;

        case 'c':
        case 'C':
          resType = ITarget::ResumeType::CONTINUE;
          break;

        case 's':
        case 'S':
          resType = ITarget::ResumeType::STEP;
          break;

        default:
          pkt.packStr ("E01");
          rsp->putPkt (pkt);
          return;
        }

      // If the core is no longer live, but has been asked to step or
      // continue, then we ignore such requests for now.
      if (resType != ITarget::ResumeType::NONE
          && !mCoreManager[i].isLive ())
        {
          cerr << "Warning: Core " << dec << i << " already exited, "
               << "ignoring request to: " << resType << endl;
          resType = ITarget::ResumeType::NONE;
        }

      mCoreManager[i].setResumeType (resType);
      coreActions.push_back (resType);
    }

  if (coreActions.size () != mCoreManager.getCpuCount ())
    {
      cerr << "*** ABORT ***: missmatch between action and core count ("
           << dec << coreActions.size () << " vs "
           << mCoreManager.getCpuCount () << ")" << endl;
      abort ();
    }

  /* Setup all the cores ready to carry out the prescribed actions.  */
  cpu->prepare (coreActions);
  doCoreActions ();
}

//! Handle a 'vKill:pid' packet.
//
//! This packet will be used in preference to the older 'k' when GDB
//! support is available.

void
GdbServer::rspVKill ()
{
  Ptid ptid (PID_DEFAULT, TID_DEFAULT);
  unsigned int pid;
  const char *str = &(pkt.data[strlen ("vKill;")]);

  if (!Utils::isHexStr (str, strlen (str)))
    {
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  pid = Utils::hex2Val (str, strlen (str));

  if (!mCoreManager.killCoreNum (CoreManager::pid2CoreNum (pid)))
    {
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  pkt.packStr ("OK");
  rsp->putPkt (pkt);

  if (mCoreManager.getLiveCoreCount () == 0)
    {
      rsp->rspClose ();
      if (killBehaviour == KillBehaviour::EXIT_ON_KILL)
          mExitServer = true;
    }
}


//! Handle a RSP 'v' packet

//! @todo for now we don't handle V packets.

void
GdbServer::rspVpkt ()
{
  if (0 == strcmp ("vCont?", pkt.data))
    {
      // What actions are supported in vCont?  If we don't support 'c' and
      // 'C' then GDB will refuse to use vCont.  If we're going to claim
      // 'C' then we may as well claim 'S' too.  I don't claim 't' yet,
      // though we probably will want that in time.
      pkt.packStr ("vCont;c;C;s;S");
      pkt.setLen (strlen (pkt.data));
      rsp->putPkt (pkt);
    }
  else if (0 == strncmp ("vCont", pkt.data, strlen ("vCont")))
    {
      rspVCont ();
      return;
    }
  else if (0 == strncmp ("vKill;", pkt.data, strlen ("vKill;")))
    {
      rspVKill ();
      return;
    }
  else
    {
      // Unsupported packet.
      pkt.packStr ("");
      rsp->putPkt (pkt);
    }
}	// rspVpkt ()


//! Handle a RSP write memory (binary) request

//! Syntax is:

//!   X<addr>,<length>:

//! Followed by the specified number of bytes as raw binary. Response should be
//! "OK" if all copied OK, E<nn> if error <nn> has occurred.

//! The length given is the number of bytes to be written. The data buffer has
//! already been unescaped, so will hold this number of bytes.

void
GdbServer::rspWriteMemBin ()
{
  uint32_t  addr;			// Where to write the memory
  std::size_t len;			// Number of bytes to write

  if (2 != sscanf (pkt.data, "X%x,%lx:", &addr, &len))
    {
      cerr << "Warning: Failed to recognize RSP write memory command: "
	   << pkt.data << endl;
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Find the start of the data and "unescape" it.
  uint8_t *bindat = (uint8_t *)(memchr (pkt.data, ':',
					pkt.getBufSize ())) + 1;
  int   off       = (char *)bindat - pkt.data;
  std::size_t newLen
    = Utils::rspUnescape ((char *)bindat, pkt.getLen () - off);

  // Sanity check
  if (newLen != len)
    {
      int  minLen = len < newLen ? len : newLen;

      cerr << "Warning: Write of " << len << " bytes requested, but "
	   << newLen << " bytes supplied. " << minLen << " will be written"
	   << endl;
      len = minLen;
    }

  // Write the bytes to memory.
  if (len != cpu->write (addr, bindat, len))
    cerr << "Warning: Failed to write " << len << " bytes to 0x" << hex
	 << addr << dec << endl;

  pkt.packStr ("OK");
  rsp->putPkt (pkt);

}	// rspWriteMemBin ()


//! Handle a RSP remove breakpoint or matchpoint request

//! This checks that the matchpoint was actually set earlier. For software
//! (memory) breakpoints, the breakpoint is cleared from memory.

//! @todo This doesn't work with icache/immu yet

void
GdbServer::rspRemoveMatchpoint ()
{
  MpType    type;			// What sort of matchpoint
  uint32_t  addr;			// Address specified
  uint32_t  instr;			// Instruction value found
  std::size_t len;			// Matchpoint length
  uint8_t  *instrVec;			// Instruction as byte vector

  pkt.packStr ("");
  rsp->putPkt (pkt);
  return;

  // Break out the instruction
  string ui32Fmt = SCNx32;
  string fmt = "z%1d,%" + ui32Fmt + ",%1d";
  if (3 != sscanf (pkt.data, fmt.c_str(), (int *)&type, &addr, &len))
    {
      cerr << "Warning: RSP matchpoint deletion request not "
	   << "recognized: ignored" << endl;
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Sanity check len
  if (len > sizeof (instr))
    {
      cerr << "Warning: RSP remove breakpoint instruction length " << len
	   << " exceeds maximum of " << sizeof (instr) << endl;
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Sort out the type of matchpoint
  switch (type)
    {
    case BP_MEMORY:
      // Software (memory) breakpoint
      if (mpHash.remove (type, addr, &instr))
	{
	  if (traceFlags->traceRsp())
	    {
	      cout << "RSP trace: software (memory) breakpoint removed from 0x"
		   << hex << addr << dec << endl;
	    }
	}
      else
	{
	  cerr << "Warning: failed to remove software (memory) breakpoint "
	          "from 0x" << hex << addr << dec << endl;
	  pkt.packStr ("E01");
	  rsp->putPkt (pkt);
	}

      if (traceFlags->traceBreak ())
	cerr << "Putting back the instruction (0x" << hex << setfill ('0')
	     << setw (4) << instr << ") at 0x" << setw(8) << addr
	     << setfill (' ')  << setw (0) << dec << endl;

      // Remove the breakpoint from memory. The endianness of the instruction
      // matches that of the memory.
      instrVec = reinterpret_cast<uint8_t *> (&instr);

      if (len != cpu->write (addr, instrVec, len))
	cerr << "Warning: Failed to write memory removing breakpoint" << endl;

      pkt.packStr ("OK");
      rsp->putPkt (pkt);
      return;

    case BP_HARDWARE:
      // Hardware breakpoint
      if (mpHash.remove (type, addr, &instr))
	{
	  if (traceFlags->traceRsp())
	    {
	      cout << "Rsp trace: hardware breakpoint removed from 0x NOT IMPLEMENTED"
		   << hex << addr << dec << endl;
	    }
      /*
	  cpu->removeBreak(addr);
	  */
	  pkt.packStr ("OK");
	  rsp->putPkt (pkt);
	}
      else
	{
	  cerr << "Warning: failed to remove hardware breakpoint from 0x"
	       << hex << addr << dec << endl;
	  pkt.packStr ("E01");
	  rsp->putPkt (pkt);
	}

      return;

    case WP_WRITE:
      // Write watchpoint
      if (mpHash.remove (type, addr, &instr))
	{
	  if (traceFlags->traceRsp())
	    {
	      cout << "RSP trace: write watchpoint removed from 0x"
		   << hex << addr << dec << endl;
	    }

	  pkt.packStr ("");		// TODO: Not yet implemented
	  rsp->putPkt (pkt);
	}
      else
	{
	  cerr << "Warning: failed to remove write watchpoint from 0x"
	       << hex << addr << dec << endl;
	  pkt.packStr ("E01");
	  rsp->putPkt (pkt);
	}

      return;

    case WP_READ:
      // Read watchpoint
      if (mpHash.remove (type, addr, &instr))
	{
	  if (traceFlags->traceRsp())
	    {
	      cout << "RSP trace: read watchpoint removed from 0x"
		   << hex << addr << dec << endl;
	    }

	  pkt.packStr ("");		// TODO: Not yet implemented
	  rsp->putPkt (pkt);
	}
      else
	{
	  cerr << "Warning: failed to remove read watchpoint from 0x"
	       << hex << addr << dec << endl;
	  pkt.packStr ("E01");
	  rsp->putPkt (pkt);
	}

      return;

    case WP_ACCESS:
      // Access (read/write) watchpoint
      if (mpHash.remove (type, addr, &instr))
	{
	  if (traceFlags->traceRsp())
	    {
	      cout << "RSP trace: access (read/write) watchpoint removed "
		      "from 0x" << hex << addr << dec << endl;
	    }

	  pkt.packStr ("");		// TODO: Not yet implemented
	  rsp->putPkt (pkt);
	}
      else
	{
	  cerr << "Warning: failed to remove access (read/write) watchpoint "
	          "from 0x" << hex << addr << dec << endl;
	  pkt.packStr ("E01");
	  rsp->putPkt (pkt);
	}

      return;

    default:
      cerr << "Warning: RSP matchpoint type " << type
	   << " not recognized: ignored" << endl;
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }
}	// rspRemoveMatchpoint ()


//! Handle a RSP insert breakpoint or matchpoint request

//! @todo For now only memory breakpoints are handled

void
GdbServer::rspInsertMatchpoint ()
{
  MpType    type;			// What sort of matchpoint
  uint32_t  addr;			// Address specified
  uint32_t  instr;			// Instruction value found
  std::size_t len;			// Matchpoint length
  uint8_t  *instrVec;			// Instruction as byte vector

  pkt.packStr ("");
  rsp->putPkt (pkt);
  return;

  // Break out the instruction
  string ui32Fmt = SCNx32;
  string fmt = "Z%1d,%" + ui32Fmt + ",%1d";
  if (3 != sscanf (pkt.data, fmt.c_str(), (int *)&type, &addr, &len))
    {
      cerr << "Warning: RSP matchpoint insertion request not "
	   << "recognized: ignored" << endl;
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Sanity check len
  if (len > sizeof (instr))
    {
      cerr << "Warning: RSP set breakpoint instruction length " << len
	   << " exceeds maximum of " << sizeof (instr) << endl;
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Sort out the type of matchpoint
  switch (type)
    {
    case BP_MEMORY:
      // Software (memory) breakpoint. Extract the instruction.
      instrVec = reinterpret_cast<uint8_t *> (&instr);

      if (len != cpu->read (addr, instrVec, len))
	cerr << "Warning: Failed to read memory when inserting breakpoint"
	     << endl;

      // Record the breakpoint and write a breakpoint instruction in its
      // place.
      mpHash.add (type, addr, instr);

      if (traceFlags->traceBreak ())
	cerr << "Inserting a breakpoint over the  instruction (0x" << hex
	     << setfill ('0') << setw (4) << instr << ") at 0x" << setw(8)
	     << addr << setfill (' ')  << setw (0) << dec << endl;

      // Little-endian, so least significant byte is at "little" address.

      instr = BREAK_INSTR;
      instrVec = reinterpret_cast<uint8_t *> (&instr);

      if (4 != cpu->write (addr, instrVec, 4))
	cerr << "Warning: Failed to write BREAK instruction" << endl;

      if (traceFlags->traceRsp())
	{
	  cout << "RSP trace: software (memory) breakpoint inserted at 0x"
	       << hex << addr << dec << endl;
	}

      pkt.packStr ("OK");
      rsp->putPkt (pkt);
      return;

    case BP_HARDWARE:
      // Hardware breakpoint
      mpHash.add (type, addr, 0);	// No instr for HW matchpoints

      if (traceFlags->traceRsp())
	{
	  cout << "RSP trace: hardware breakpoint set at 0x NOT IMPLEMENTED"
	       << hex << addr << dec << endl;
	}
      /*
      cpu->insertBreak(addr);
      */
      pkt.packStr ("OK");
      rsp->putPkt (pkt);

      return;

    case WP_WRITE:
      // Write watchpoint
      mpHash.add (type, addr, 0);	// No instr for HW matchpoints

      if (traceFlags->traceRsp())
	{
	  cout << "RSP trace: write watchpoint set at 0x"
	       << hex << addr << dec << endl;
	}

      pkt.packStr ("");		// TODO: Not yet implemented
      rsp->putPkt (pkt);

      return;

    case WP_READ:
      // Read watchpoint
      mpHash.add (type, addr, 0);	// No instr for HW matchpoints

      if (traceFlags->traceRsp())
	{
	  cout << "RSP trace: read watchpoint set at 0x"
	       << hex << addr << dec << endl;
	}

      pkt.packStr ("");		// TODO: Not yet implemented
      rsp->putPkt (pkt);

      return;

    case WP_ACCESS:
      // Access (read/write) watchpoint
      mpHash.add (type, addr, 0);	// No instr for HW matchpoints

      if (traceFlags->traceRsp())
	{
	  cout << "RSP trace: access (read/write) watchpoint set at 0x"
	       << hex << addr << dec << endl;
	}

      pkt.packStr ("");		// TODO: Not yet implemented
      rsp->putPkt (pkt);

      return;

    default:
      cerr << "Warning: RSP matchpoint type " << type
	   << "not recognized: ignored"<< endl;
      pkt.packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }
}	// rspInsertMatchpoint ()


//! Output operator for TargetSignal enumeration

//! @param[in] s  The stream to output to.
//! @param[in] p  The TargetSignal value to output.
//! @return  The stream with the item appended.

std::ostream &
operator<< (std::ostream & s,
	    GdbServer::TargetSignal  p)
{
  const char * name;

  switch (p)
    {
    case GdbServer::TargetSignal::NONE:    name = "SIGNONE";    break;
    case GdbServer::TargetSignal::INT:     name = "SIGINT";     break;
    case GdbServer::TargetSignal::TRAP:    name = "SIGTRAP";    break;
    case GdbServer::TargetSignal::USR1:    name = "SIGUSR1";    break;
    case GdbServer::TargetSignal::XCPU:    name = "SIGXCPU";    break;
    case GdbServer::TargetSignal::UNKNOWN: name = "SIGUNKNOWN"; break;
    default:                                   name = "unknown";    break;
    }

  return  s << name;

}	// operator<< ()


//! Output operator for StopMode enumeration

//! @param[in] s  The stream to output to.
//! @param[in] p  The StopMode value to output.
//! @return  The stream with the item appended.

std::ostream &
operator<< (std::ostream & s,
	    GdbServer::StopMode  p)
{
  const char * name;

  switch (p)
    {
    case GdbServer::StopMode::NON_STOP: name = "NON_STOP"; break;
    case GdbServer::StopMode::ALL_STOP: name = "ALL_STOP"; break;
    default:                                name = "unknown";  break;
    }

  return  s << name;

}	// operator<< ()


//! Constructor for CoreManager class
//
//! Setup data structures to track 'count' cores.

GdbServer::CoreManager::CoreManager (unsigned int count)
  : mNumCores (count),
    mLiveCores (count)
{
  mCoreStates.resize (count);
}

//! Reset the core manager, restoring all cores to life.
//
//! Any exited cores are once again alive, and non-exited after a call to
//! the reset method.

void
GdbServer::CoreManager::reset ()
{
  mLiveCores = mNumCores;

  // First resize to zero to delete all of the core status objects, then
  // grow the array again.  This will reinitialise all of the core
  // statuses.
  mCoreStates.resize (0);
  mCoreStates.resize (mNumCores);
}

//! Mark 'coreNum' as killed (or exited)
//
//! Returns true if coreNum was successfully marked as killed, otherwise
//! returns false (for example if there is no 'coreNum'.  If 'coreNum' was
//! already killed then the core remains killed and we return true.

bool
GdbServer::CoreManager::killCoreNum (unsigned int coreNum)
{
  if (coreNum < mNumCores)
    {
      mCoreStates[coreNum].killCore ();
      --mLiveCores;
      return true;
    }

  return false;
}


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
