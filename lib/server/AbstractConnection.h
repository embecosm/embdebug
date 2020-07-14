// Remote Serial Protocol connection: declaration
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2017-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef ABSTRACT_CONNECTION_H
#define ABSTRACT_CONNECTION_H

#include "RspPacket.h"
#include "TraceFlags.h"

namespace EmbDebug {

//! Class implementing the RSP connection listener

//! This class is entirely passive. It is up to the caller to determine that a
//! packet will become available before calling ::getPkt ().

class AbstractConnection {
public:
  // Constructor and Destructor

  AbstractConnection(TraceFlags *_traceFlags);
  virtual ~AbstractConnection() = 0;

  // Public interface: manage client connections

  virtual bool rspConnect() = 0;
  virtual void rspClose() = 0;
  virtual bool isConnected() = 0;

  // Public interface: get packets from the stream and put them out

  virtual std::pair<bool, RspPacket> getPkt();
  virtual bool putPkt(const RspPacket &pkt);

  // Check for a break (ctrl-C)

  virtual bool haveBreak();

  // Disable packet acknowledgements
  void setNoAckMode(bool ackMode) { mNoAckMode = ackMode; }

protected:
  //! Trace flags

  TraceFlags *traceFlags;

  // Internal OS specific routines to handle individual chars.

  virtual bool putRspCharRaw(char c) = 0;
  virtual int getRspCharRaw(bool blocking) = 0;

private:
  //! The BREAK character

  static const int BREAK_CHAR = 3;

  //! Has a BREAK arrived?

  bool mHavePendingBreak;

  //! Is the server in NoAckMode

  bool mNoAckMode;

  //! The buffered char for get RspChar
  int mGetCharBuf;

  //! Count of how many buffered chars we have
  int mNumGetBufChars;

  // Internal routines to handle individual chars

  bool putRspChar(char c);
  int getRspChar();
};

// Default implementation of the destructor.

inline AbstractConnection::~AbstractConnection() {}

inline AbstractConnection::AbstractConnection(TraceFlags *_traceFlags)
    : traceFlags(_traceFlags), mHavePendingBreak(false), mNoAckMode(false),
      mNumGetBufChars(0) {}

} // namespace EmbDebug

#endif
