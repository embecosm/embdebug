// Remote Serial Protocol connection: implementation

// Copyright (C) 2009, 2013  Embecosm Limited <info@embecosm.com>

// Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

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

#include <fstream>
#include <iomanip>
#include <iostream>

#include <cerrno>
#include <csignal>
#include <cstring>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "RspConnection.h"
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
RspConnection::RspConnection(int _portNum, TraceFlags *_traceFlags,
                             bool _writePort)
    : AbstractConnection(_traceFlags), portNum(_portNum), clientFd(-1),
      writePort(_writePort) {} // RspConnection ()

//! Destructor

//! Close the connection if it is still open
RspConnection::~RspConnection() {
  this->rspClose(); // Don't confuse with any other close ()

} // ~RspConnection ()

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
bool RspConnection::rspConnect() {
  // Open a socket on which we'll listen for clients
  int tmpFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (tmpFd < 0) {
    cerr << "ERROR: Cannot open RSP socket" << endl;
    return false;
  }

  // Allow rapid reuse of the port on this socket
  int optval = 1;
  setsockopt(tmpFd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));

  // Bind the port to the socket
  struct sockaddr_in sockAddr;
  sockAddr.sin_family = PF_INET;
  sockAddr.sin_port = htons(portNum);
  sockAddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(tmpFd, (struct sockaddr *)&sockAddr, sizeof(sockAddr))) {
    cerr << "ERROR: Cannot bind to RSP socket" << endl;
    return false;
  }

  // Listen for (at most one) client
  if (listen(tmpFd, 1)) {
    cerr << "ERROR: Cannot listen on RSP socket" << endl;
    return false;
  }

  // If port 0 specified, determine which port we were assigned
  if (portNum == 0) {
    socklen_t len = sizeof(sockAddr);
    getsockname(tmpFd, (struct sockaddr *)&sockAddr, &len);
    portNum = ntohs(sockAddr.sin_port);
  }

  if (!traceFlags->traceSilent())
    cout << "Listening for RSP on port " << portNum << endl << flush;

  if (writePort) {
    // Generate a file to signal that the Gdbserver side is ready
    std::ofstream fs;
    fs.open("simulation_ready.txt");
    fs << portNum << endl;
    fs.close();
  }
  // Accept a client which connects
  socklen_t len = sizeof(sockAddr); // Size of the socket address
  clientFd = accept(tmpFd, (struct sockaddr *)&sockAddr, &len);

  if (-1 == clientFd) {
    cerr << "Warning: Failed to accept RSP client: " << strerror(errno) << endl;
    return true; // OK to retry
  }

  // Enable TCP keep alive process
  optval = 1;
  setsockopt(clientFd, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval,
             sizeof(optval));

  // Don't delay small packets, for better interactive response (disable
  // Nagel's algorithm)
  optval = 1;
  setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, (char *)&optval,
             sizeof(optval));

  // Socket is no longer needed
  close(tmpFd);             // No longer need this
  signal(SIGPIPE, SIG_IGN); // So we don't exit if client dies

  if (!traceFlags->traceSilent())
    cout << "Remote debugging from host " << inet_ntoa(sockAddr.sin_addr)
         << endl;

  return true;

} // rspConnect ()

//! Close a client connection if it is open
void RspConnection::rspClose() {
  if (isConnected()) {
    if (!traceFlags->traceSilent())
      cout << "Closing connection" << endl;

    close(clientFd);
    clientFd = -1;
  }
} // rspClose ()

//! Report if we are connected to a client.

//! @return  TRUE if we are connected, FALSE otherwise
bool RspConnection::isConnected() { return -1 != clientFd; } // isConnected ()

//! Put a single character out on the RSP connection

//! Utility routine. This should only be called if the client is open, but we
//! check for safety.

//! @param[in] c         The character to put out

//! @return  TRUE if char sent OK, FALSE if not (communications failure)

bool RspConnection::putRspCharRaw(char c) {
  if (-1 == clientFd) {
    cerr << "Warning: Attempt to write '" << c
         << "' to unopened RSP client: Ignored" << endl;
    return false;
  }

  // Write until successful (we retry after interrupts) or catastrophic
  // failure.
  while (true) {
    switch (write(clientFd, &c, sizeof(c))) {
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

int RspConnection::getRspCharRaw(bool blocking) {
  if (-1 == clientFd) {
    cerr << "Warning: Attempt to read from "
         << "unopened RSP client: Ignored" << endl;
    return -1;
  }

  // Blocking read until successful (we retry after interrupts) or
  // catastrophic failure.

  for (;;) {
    unsigned char c;

    switch (recv(clientFd, &c, sizeof(c), (blocking ? 0 : MSG_DONTWAIT))) {
    case -1:
      if (!blocking && (errno == EAGAIN || errno == EWOULDBLOCK))
        return -1;

      // Error: only allow interrupts

      if (EINTR != errno) {
        cerr << "Warning: Failed to read from RSP client: "
             << "Closing client connection: " << strerror(errno) << endl;
        return -1;
      }
      break;

    case 0:
      return -1;

    default:
      return c & 0xff; // Success, we can return (no sign extend!)
    }
  }
} // getRspCharRaw ()
