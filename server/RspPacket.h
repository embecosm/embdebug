// RSP packet: definition
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef RSP_PACKET_H
#define RSP_PACKET_H

#include <iostream>

namespace EmbDebug {

//! Class for RSP packets

//! Can't be null terminated, since it may include zero bytes
class RspPacket {
public:
  //! The data buffer. Allow direct access to avoid unnecessary copying.
  char *data;

  // Constructor and destructor
  RspPacket(std::size_t _bufSize);
  RspPacket(RspPacket &&other);
  RspPacket(const RspPacket &other) = delete;
  ~RspPacket();

  RspPacket &operator=(const RspPacket &other) = delete;
  RspPacket &operator=(RspPacket &&other);

  // Pack a constant string into a packet
  void packStr(const char *str);    // For fixed packets
  void packRcmdStr(const char *str, // For qRcmd replies
                   const bool toStdoutP);

  // Pack a hex encoded string into a packet
  void packHexstr(const char *str);

  // Accessors
  std::size_t getBufSize();
  std::size_t getLen();
  void setLen(std::size_t _len);

private:
  //! The data buffer size
  std::size_t bufSize;

  //! Number of chars in the data buffer (<= bufSize)
  std::size_t len;
};

//! Stream output
std::ostream &operator<<(std::ostream &s, RspPacket &p);

} // namespace EmbDebug

#endif
