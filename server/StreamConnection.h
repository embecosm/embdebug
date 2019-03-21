// Remote Serial Protocol connection: declaration
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2017-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef STREAM_CONNECTION_H
#define STREAM_CONNECTION_H

#include "AbstractConnection.h"
#include "embdebug/TraceFlags.h"

namespace EmbDebug {

//! Class implementing the RSP connection listener

//! This class is entirely passive. It is up to the caller to determine
//! that a packet will become available before calling ::getPkt ().

class StreamConnection : public AbstractConnection {
public:
  // Constructors and destructor

  StreamConnection(TraceFlags *_traceFlags);
  ~StreamConnection();

  // Public interface: manage client connections

  virtual bool rspConnect();
  virtual void rspClose();
  virtual bool isConnected();

private:
  // Implementation specific routines to handle individual chars.

  virtual bool putRspCharRaw(char c);
  virtual int getRspCharRaw(bool blocking);

  // Track whether we are connected or not.
  bool mIsConnected;
}; // StreamConnection ()

} // namespace EmbDebug

#endif // STREAM_CONNECTION_H
