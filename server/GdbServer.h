// GDB RSP server implementation: definition
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef GDB_SERVER_H
#define GDB_SERVER_H

#define __STDC_FORMAT_MACROS
#include <cassert>
#include <cinttypes>
#include <map>
#include <vector>

#include "Ptid.h"
#include "RspPacket.h"
#include "embdebug/ITarget.h"
#include "embdebug/Timeout.h"
#include "embdebug/Types.h"

namespace EmbDebug {

class AbstractConnection;

//! Module implementing a GDB RSP server.

//! How should we behave when GDB sends a kill (k) packet?
enum KillBehaviour {
  //! Reset the target, but remain alive.
  RESET_ON_KILL,
  //! Stop the target, close the connection and return.
  EXIT_ON_KILL
};

//! A loop listens for RSP requests, which are converted to requests to read
//! and write registers, read and write memory, or control the CPU

class GdbServer {
public:
  // Constructor and destructor

  GdbServer(AbstractConnection *_conn, ITarget *_cpu, TraceFlags *traceFlags,
            KillBehaviour _killBehaviour);
  ~GdbServer();

  // Main loop to listen for and service RSP requests.

  int rspServer();

private:
  //! Definition of GDB target signals.

  enum class TargetSignal : int {
    NONE = 0,
    INT = 2,
    TRAP = 5,
    XCPU = 24,
    USR1 = 30,
    UNKNOWN = 143
  };

  //! What stop mode we are in

  enum class StopMode : char { NON_STOP, ALL_STOP };

  //! Enumeration of different types of matchpoint.

  //! These have explicit values matching the second digit of 'z' and 'Z'
  //! packets.
  enum class MatchpointType : int {
    BP_MEMORY = 0,
    BP_HARDWARE = 1,
    WP_WRITE = 2,
    WP_READ = 3,
    WP_ACCESS = 4
  };

  // stream operators have to be friends to access private members

  friend std::ostream &operator<<(std::ostream &s, TargetSignal p);

  friend std::ostream &operator<<(std::ostream &s, StopMode p);

  friend std::ostream &operator<<(std::ostream &s, MatchpointType p);

  // For now these are hard-coded constants, but they need to be made
  // configurable.

  // Default values for PTIDs.

  static const int PID_DEFAULT = 1; //!< Default PID is core 0
  static const int TID_DEFAULT = 1; //!< Only ever have one thread

  //! Constant for a breakpoint (EBREAK). Remember we are little-endian.

  static const uint32_t BREAK_INSTR = 0x100073;

  //! Constant which is the sample period (in instruction steps) during
  //! "continue" etc.

  static const int RUN_SAMPLE_PERIOD = 10000;

  //! Our associated simulated CPU

  ITarget *cpu;

  //! Our trace flags

  TraceFlags *traceFlags;

  //! Our associated RSP interface

  AbstractConnection *rsp;

  //! The number of registers in the CPU
  int mNumRegs;

  //! The packet pointer. There is only ever one packet in use at one time, so
  //! there is no need to repeatedly allocate and delete it.

  RspPacket pkt;

  //! Hash table for matchpoints

  std::map<std::pair<MatchpointType, uint_addr_t>, uint64_t> mMatchpointMap;

  //! Timeout for continue.

  Timeout mTimeout;

  //! How to behave when we get a kill (k) packet.

  KillBehaviour killBehaviour;

  //! Whether some cause for exit from the server has arisen

  bool mExitServer;

  //! Whether the client supports multiprocess

  bool mHaveMultiProc;

  //! Stop mode

  StopMode mStopMode;

  //! Current PTID

  Ptid mPtid;

  //! Next process to report, this is only used when responding to
  //! qfThreadInfo and qsThreadInfo requests.

  unsigned int mNextProcess;

  //! Track when we are processing a syscall.  We shouldn't get nested
  //! syscalls.

  bool mHandlingSyscall;

  bool mHaveSyscallArgLocs;
  bool mHaveSyscallSupport;
  ITarget::SyscallArgLoc mSyscallIDLoc;
  ITarget::SyscallArgLoc mSyscallReturnLoc;
  std::vector<ITarget::SyscallArgLoc> mSyscallArgLocs;

  //! When this is true, cores are marked as killed when they perform an
  //! exit syscall.  When it is false, the core remains alive, in which
  //! case it looks (to GDB) like a new inferior has immediately spawned to
  //! take the place of the exited inferior.
  //!
  //! The default is currently false for this, as this actually provides
  //! the nicer GDB experience.
  bool mKillCoreOnExit;

  //! Class to keep track of the number of cores on the machine, and how
  //! many are still alive.

  class CoreManager {
  public:
    CoreManager(unsigned int count);

    unsigned int getCpuCount() const { return mNumCores; }

    unsigned int getLiveCoreCount() const { return mLiveCores; }

    static unsigned int pid2CoreNum(unsigned int pid) { return pid - 1; }

    static unsigned int coreNum2Pid(unsigned int coreNum) {
      return coreNum + 1;
    }

    bool isCoreLive(unsigned int coreNum) const {
      return mCoreStates[coreNum].isLive();
    }

    bool killCoreNum(unsigned int coreNum);

    void reset();

    //! Class to keep track of the current state of one target core.

    class CoreState {
    public:
      CoreState()
          : mStopReason(ITarget::ResumeRes::INTERRUPTED),
            mResumeType(ITarget::ResumeType::NONE), mStopReported(true),
            mIsLive(true) {}

      void killCore() { mIsLive = false; }

      bool isLive() const { return mIsLive; }

      ITarget::ResumeRes stopReason() const { return mStopReason; }

      bool isRunning() const {
        return mResumeType != ITarget::ResumeType::NONE;
      }

      bool hasUnreportedStop() const { return !mStopReported; }

      void reportStopReason() { mStopReported = true; }

      void setStopReason(ITarget::ResumeRes res) {
        mStopReason = res;
        mStopReported = (res == ITarget::ResumeRes::NONE);
      }

      void setResumeType(ITarget::ResumeType type) { mResumeType = type; }

    private:
      // The last reason that this core stopped.
      ITarget::ResumeRes mStopReason;

      // The last "run" action that was applied to this core.
      ITarget::ResumeType mResumeType;

      // Is true when the last stop reason has been reported to GDB, false
      // if this event is still pending.
      bool mStopReported;

      // True when this core is "live", set to false when the core calls
      // exit, after which the core is considered "not-live".
      bool mIsLive;
    };

    CoreState &operator[](std::size_t idx) {
      assert(idx < mNumCores);
      return mCoreStates[idx];
    }

    const CoreState &operator[](std::size_t idx) const {
      assert(idx < mNumCores);
      return mCoreStates[idx];
    }

  private:
    // Delete default and copy constructors.
    CoreManager() = delete;
    CoreManager(const CoreManager &) = delete;

    //! Total number of cores.
    unsigned int mNumCores;

    //! Number of cores that are still live.  Should correspond to the
    //! number of entries in mCoreLive that are true.
    unsigned int mLiveCores;

    std::vector<CoreState> mCoreStates;
  };

  //! Keep track of core count, and which cores are live.
  CoreManager mCoreManager;

protected:
  // Main RSP request handler
  void rspClientRequest();

private:
  // Handle the various RSP requests
  uint_reg_t readArgLoc(const ITarget::SyscallArgLoc &loc);
  int stringLength(uint_addr_t addr);
  void rspSyscallRequest();
  void rspSyscallReply();
  void rspReportException(TargetSignal sig = TargetSignal::TRAP);
  void rspReadAllRegs();
  void rspWriteAllRegs();
  void rspReadMem();
  void rspWriteMem();
  void rspReadReg();
  void rspWriteReg();
  void rspQuery();
  void rspCommand();
  void rspSetCommand(const char *cmd);
  void rspShowCommand(const char *cmd);
  void rspSet();
  void rspRestart();
  void rspVpkt();
  void rspWriteMemBin();
  void rspRemoveMatchpoint();
  void rspInsertMatchpoint();
  void rspWriteNextThreadInfo();
  void rspVCont();
  void rspVKill();

  void doCoreActions(void);
  bool getNextStopEvent(unsigned int &, ITarget::ResumeRes &);
  bool processStopEvents(void);
};

} // namespace EmbDebug

#endif
