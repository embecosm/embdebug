// Remote Serial Protocol connection: implementation
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include <fstream>
#include <iomanip>
#include <iostream>

#include <cerrno>
#include <csignal>
#include <cstring>

#include <WS2tcpip.h>
#include <io.h>
#include <winsock2.h>
typedef int socklen_t;

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
    : AbstractConnection(_traceFlags), portNum(_portNum),
      clientSock(INVALID_SOCKET), writePort(_writePort) {
  // Initialize Winsock 2.2.
  WSAData wsaData;
  if (int error = WSAStartup(MAKEWORD(2, 2), &wsaData)) {
    cerr << "Warning: WSAStartup failed with error " << error << "." << endl;
  }
}

//! Destructor

//! Close the connection if it is still open
RspConnection::~RspConnection() {
  this->rspClose(); // Don't confuse with any other close ()
  WSACleanup();
}

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
  SOCKET tmpSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (tmpSock == INVALID_SOCKET) {
    cerr << "ERROR: Cannot open RSP socket" << endl;
    return false;
  }

  // Allow rapid reuse of the port on this socket
  int optval = 1;
  setsockopt(tmpSock, SOL_SOCKET, SO_REUSEADDR, (char *)&optval,
             sizeof(optval));

  // Bind the port to the socket
  struct sockaddr_in sockAddr;
  sockAddr.sin_family = PF_INET;
  sockAddr.sin_port = htons(portNum);
  sockAddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(tmpSock, (struct sockaddr *)&sockAddr, sizeof(sockAddr))) {
    cerr << "ERROR: Cannot bind to RSP socket" << endl;
    return false;
  }

  // Listen for (at most one) client
  if (listen(tmpSock, 1)) {
    cerr << "ERROR: Cannot listen on RSP socket" << endl;
    return false;
  }

  // If port 0 specified, determine which port we were assigned
  if (portNum == 0) {
    socklen_t len = sizeof(sockAddr);
    getsockname(tmpSock, (struct sockaddr *)&sockAddr, &len);
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
  clientSock = accept(tmpSock, (struct sockaddr *)&sockAddr, &len);

  if (!isConnected()) {
    cerr << "Warning: Failed to accept RSP client: " << WSAGetLastError()
         << endl;
    return true; // OK to retry
  }

  // Enable TCP keep alive process
  optval = 1;
  setsockopt(clientSock, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval,
             sizeof(optval));

  // Don't delay small packets, for better interactive response (disable
  // Nagel's algorithm)
  optval = 1;
  setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, (char *)&optval,
             sizeof(optval));

  // Socket is no longer needed
  closesocket(tmpSock); // No longer need this

  if (!traceFlags->traceSilent()) {
    char str[INET_ADDRSTRLEN];
    cout << "Remote debugging from host "
         << inet_ntop(AF_INET, &sockAddr.sin_addr, str, INET_ADDRSTRLEN)
         << endl;
  }

  return true;
}

//! Close a client connection if it is open
void RspConnection::rspClose() {
  if (isConnected()) {
    if (!traceFlags->traceSilent())
      cout << "Closing connection" << endl;

    // Attempt to shutdown connection before closing socket
    if (shutdown(clientSock, SD_SEND) == SOCKET_ERROR)
      cerr << "Warning: shutdown failed: " << WSAGetLastError() << endl;

    closesocket(clientSock);
    clientSock = INVALID_SOCKET;
  }
}

//! Report if we are connected to a client.

//! @return  TRUE if we are connected, FALSE otherwise
bool RspConnection::isConnected() { return INVALID_SOCKET != clientSock; }

//! Put a single character out on the RSP connection

//! Utility routine. This should only be called if the client is open, but we
//! check for safety.

//! @param[in] c         The character to put out

//! @return  TRUE if char sent OK, FALSE if not (communications failure)

bool RspConnection::putRspCharRaw(char c) {
  if (clientSock == INVALID_SOCKET) {
    cerr << "Warning: Attempt to write '" << c
         << "' to unopened RSP client: Ignored" << endl;
    return false;
  }

  // Attempt to write to the socket
  if (send(clientSock, &c, sizeof(c), 0) == SOCKET_ERROR) {
    cerr << "Warning: Failed to write to RSP Client: "
         << "Closing client connection: " << WSAGetLastError() << endl;
    rspClose();
    return false;
  }
  return true;
}

//! Get a single character from the RSP connection

//! Utility routine. This should only be called if the client is open, but we
//! check for safety.

//! @param[in] blocking  True if the read should block.
//! @return  The character received or -1 on failure, or if the read would
//!          block, and blocking is true.

int RspConnection::getRspCharRaw(bool blocking) {
  if (!isConnected()) {
    cerr << "Warning: Attempt to read from "
         << "unopened RSP client: Ignored" << endl;
    return -1;
  }

  // Set the blocking mode of the client socket
  u_long blockingMode = blocking ? 0 : 1;
  if (ioctlsocket(clientSock, FIONBIO, &blockingMode))
    cerr << "Warning: Unable to set blocking mode of socket." << endl;

  // Blocking read until successful (we retry after interrupts) or
  // catastrophic failure.

  while (true) {
    char c;
    switch (recv(clientSock, &c, sizeof(c), 0)) {
    case SOCKET_ERROR:
      // If non-blocking and no data, return -1
      if (!blocking && WSAGetLastError() == WSAEWOULDBLOCK)
        return -1;

      cerr << "Warning: Failed to read from RSP client: " << WSAGetLastError()
           << endl;
      return -1;

    case 0:
      return -1;

    default:
      return c & 0xff; // Success, we can return (no sign extend!)
    }
  }
}
