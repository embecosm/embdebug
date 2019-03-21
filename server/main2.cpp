// GDBServer library entry point
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include "AbstractConnection.h"
#include "GdbServer.h"
#include "RspConnection.h"
#include "StreamConnection.h"
#include "embdebug/ITarget.h"

using namespace EmbDebug;

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
