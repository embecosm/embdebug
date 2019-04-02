// GDBServer library entry point
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include "embdebug/Init.h"
#include "AbstractConnection.h"
#include "GdbServer.h"
#include "RspConnection.h"
#include "StreamConnection.h"
#include "embdebug/ITarget.h"

using namespace EmbDebug;

int EmbDebug::init(ITarget *target, TraceFlags *traceFlags,
                   bool useStreamConnection, int rspPort, bool writePort) {
  AbstractConnection *conn;
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
