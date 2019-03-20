// GDBServer library entry point

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
// ----------------------------------------------------------------------------

#include "AbstractConnection.h"
#include "GdbServer.h"
#include "RspConnection.h"
#include "StreamConnection.h"
#include "embdebug/ITarget.h"

static ITarget *globalTargetHandle = nullptr;

int main2(ITarget *target, TraceFlags *traceFlags, bool useStreamConnection,
          int rspPort, bool writePort) {
  // Take a global reference to the target so that we can get at it
  // from sc_time_stamp
  globalTargetHandle = target;

  AbstractConnection *conn;
  ;
  KillBehaviour killBehaviour;
  if (useStreamConnection) {
    conn = new StreamConnection(traceFlags);
    killBehaviour = KillBehaviour::EXIT_ON_KILL;
  } else {
    conn = new RspConnection(rspPort, traceFlags, writePort);
    killBehaviour = KillBehaviour::RESET_ON_KILL;
  }

  // The RSP server, connecting it to its CPU.

  GdbServer gdbServer(conn, target, traceFlags, killBehaviour);

  // Run the GDB server.

  if (useStreamConnection)
    std::cout << std::endl << "READY" << std::endl << std::flush;

  int ret = gdbServer.rspServer();

  delete conn;
  return ret;
}

//! Function to handle $time calls in the Verilog

double sc_time_stamp() {
  // If we are called before cpu has been constructed, return 0.0
  if (globalTargetHandle != nullptr)
    return globalTargetHandle->timeStamp();
  else
    return 0.0;
}
