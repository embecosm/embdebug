// Remote Serial Protocol connection: declaration
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef RSP_CONNECTION_H
#define RSP_CONNECTION_H

#include "AbstractConnection.h"

// This provides a valid typedef for the Windows SOCKET type, but avoids pulling
// in winsock2.h; doing so would pollute the global and preprocessor namespaces.
#ifdef WIN32
#if defined(_WIN64)
typedef unsigned __int64 UINT_PTR;
#else
typedef _W64 unsigned int UINT_PTR;
#endif
typedef UINT_PTR SOCKET;
#endif

namespace EmbDebug {

class TraceFlags;

//! Class implementing the RSP connection listener

//! This class is entirely passive. It is up to the caller to determine that a
//! packet will become available before calling ::getPkt ().

class RspConnection : public AbstractConnection {
public:
  // Constructors and destructor

  RspConnection(int _portNum, TraceFlags *_traceFlags, bool _writePort);
  ~RspConnection();

  // Public interface: manage client connections

  bool rspConnect();
  void rspClose();
  bool isConnected();

private:
  //! The port number to listen on

  int portNum;

  //! The client file descriptor/socket

#ifdef WIN32
  SOCKET clientSock;
#else
  int clientFd;
#endif

  //! Whether to write the port number to a text file on startup

  bool writePort;

  // Implementation specific routines to handle individual chars.

  virtual bool putRspCharRaw(char c);
  virtual int getRspCharRaw(bool blocking);
};

} // namespace EmbDebug

#endif
