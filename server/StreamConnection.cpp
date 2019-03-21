// Remote Serial Protocol connection: implementation
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2017-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include <iomanip>
#include <iostream>

#include <cerrno>
#include <csignal>
#include <cstring>

#include <sys/select.h>
#include <unistd.h>

#include "StreamConnection.h"
#include "embdebug/Utils.h"

using std::cerr;
using std::cout;
using std::dec;
using std::endl;
using std::flush;
using std::hex;
using std::setfill;
using std::setw;

using namespace EmbDebug;

//! Constructor when using a port number

//! Sets up various parameters

//! @param[in] _portNum     the port number to connect to
//! @param[in] _traceFlags  flags controlling tracing
StreamConnection::StreamConnection(TraceFlags *_traceFlags)
    : AbstractConnection(_traceFlags), mIsConnected(true) {
  // Nothing.
} // StreamConnection ()

//! Destructor

//! Close the connection if it is still open
StreamConnection::~StreamConnection() {
  this->rspClose(); // Don't confuse with any other close ()
} // ~StreamConnection ()

//! Get a new client connection.

//! Blocks until the client connection is available.

//! A lot of this code is copied from remote_open in gdbserver remote-utils.c.

//! This involves setting up a socket to listen on a socket for attempted
//! connections from a single GDB instance (we couldn't be talking to multiple
//! GDBs at once!).

//! The service is specified either as a port number in the Or1ksim
//! configuration (parameter rsp_port in section debug, default 51000) or as a
//! service name in the constant OR1KSIM_RSP_SERVICE.

//! If there is a catastrophic communication failure, service will be
//! terminated using sc_stop.

//! The protocol used for communication is specified in OR1KSIM_RSP_PROTOCOL.

//! @return  TRUE if the connection was established or can be retried. FALSE
//!          if the error was so serious the program must be aborted.
bool StreamConnection::rspConnect() {
  // There's no way to connect, we rely on stdin / stdout being passe into
  // the process, we're connected from the start.  As we currently always
  // report that we're connected this should never be called.
  return false;
} // rspConnect ()

//! Close a client connection if it is open.  This is called once we detect
//! that stdin might have closed.  Remember we're now in a closed state.
void StreamConnection::rspClose() { mIsConnected = false; } // rspClose ()

//! Report if we are connected to a client.

//! @return  TRUE if we are connected, FALSE otherwise
bool StreamConnection::isConnected() { return mIsConnected; } // isConnected ()

//! Put a single character out on the RSP connection

//! Utility routine. This should only be called if the client is open, but we
//! check for safety.

//! @param[in] c         The character to put out

//! @return  TRUE if char sent OK, FALSE if not (communications failure)

bool StreamConnection::putRspCharRaw(char c) {
  // Write until successful (we retry after interrupts) or catastrophic
  // failure.
  while (true) {
    switch (write(STDOUT_FILENO, &c, sizeof(c))) {
    case -1:
      // Error: only allow interrupts or would block
      if ((EAGAIN != errno) && (EINTR != errno)) {
        cerr << "Warning: Failed to write to RSP client: "
             << "Closing client connection: " << strerror(errno) << endl;
        return false;
      }

      break;

    case 0:
      break; // Nothing written! Try again

    default:
      return true; // Success, we can return
    }
  }
} // putRspCharRaw ()

//! Get a single character from the RSP connection

//! Utility routine. This should only be called if the client is open, but we
//! check for safety.

//! @param[in] blocking  True if the read should block.
//! @return  The character received or -1 on failure, or if the read would
//!          block, and blocking is true.

int StreamConnection::getRspCharRaw(bool blocking) {
  // Blocking read until successful (we retry after interrupts) or
  // catastrophic failure.

  for (;;) {
    unsigned char c;
    int res;
    struct timeval timeout;
    fd_set readfds;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    res = select(STDIN_FILENO + 1, &readfds, NULL, NULL,
                 (blocking ? NULL : &timeout));

    switch (res) {
    case -1:
      // Error: only allow interrupts

      if (EINTR != errno) {
        cerr << "Warning: Failed to read from RSP client: "
             << "Closing client connection: " << strerror(errno) << endl;
        return -1;
      }
      break;

    case 0:
      // Timeout, only happens in the blocking case.
      return -1;

    default: {
      ssize_t count;

      if ((count = read(STDIN_FILENO, &c, sizeof(c))) == -1)
        return -1;

      if (count == 0)
        return -1;

      return c & 0xff; // Success, we can return (no sign extend!)
    }
    }
  }
} // getRspCharRaw ()
