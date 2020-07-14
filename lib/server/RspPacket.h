// RSP packet: definition
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef RSP_PACKET_H
#define RSP_PACKET_H

#include <cassert>
#include <cstdarg>
#include <iostream>

#include "embdebug/ByteView.h"
#include "embdebug/Compat.h"

namespace EmbDebug {

class RspPacketBuilder;

//! Class for RSP packets

//! Can't be null terminated, since it may include zero bytes
class RspPacket {
public:
  //! The data buffer. Allow direct access to avoid unnecessary copying.

  // Constructor and destructor
  RspPacket() = default;
  RspPacket(RspPacket &&other);
  RspPacket(const RspPacket &other) = default;
  ~RspPacket() = default;
  RspPacket(const RspPacketBuilder &builder);

  RspPacket &operator=(const RspPacket &other) = delete;
  RspPacket &operator=(RspPacket &&other) = default;

  //! Create packet from constant string
  RspPacket(const char *X) {
    len = ::strlen(X);
    ::memcpy(data, X, len);
  }

  //! Create packet from constant char buffer
  RspPacket(const char *X, std::size_t _len) {
    len = _len;
    ::memcpy(data, X, len);
  }

  //! Create packet from printf-style call
  static RspPacket CreateFormatted(const char *format, ...);

  //! Create packet in response to RCmd packet
  static RspPacket CreateRcmdStr(const char *str, const bool toStdoutP);

  //! Hex-encode packet from string
  static RspPacket CreateHexStr(const char *str);

  // Accessors
  static constexpr std::size_t getMaxPacketSize() { return bufSize; };
  std::size_t getLen() const { return len; };

  //! Access data buffer
  const char *getRawData() const { return data; }

  //! Access data buffer via ByteView
  ByteView getData() const { return ByteView(data, len); }

private:
  //! The data buffer size
  static const std::size_t bufSize = 10000;

  //! The data pointer
  char data[bufSize] = {0};

  //! Number of chars in the data buffer (<= bufSize)
  std::size_t len = 0;
};

//! RspPacket Builder

//! This provides a convenience mechanism for building up valid packets from a
//! set of chars/c strings/byte arrays
class RspPacketBuilder {
  // RspPacket can see the builders data and length buffers for constructing a
  // packet from the builders current state.
  friend class RspPacket;

  char data[RspPacket::getMaxPacketSize()] = {0};
  std::size_t len = 0;

public:
  RspPacketBuilder &operator+=(const char *str);
  RspPacketBuilder &operator+=(const char c);
  void addData(const char *str);
  void addData(const char *str, std::size_t _len);
  void addData(const ByteView view) { addData(view.getData(), view.getLen()); }

  std::size_t getSize() const { return len; }
  std::size_t getRemaining() const {
    return RspPacket::getMaxPacketSize() - len;
  }

  void erase() {
    ::memset(data, 0, RspPacket::getMaxPacketSize());
    len = 0;
  }
};

//! Stream output
std::ostream &operator<<(std::ostream &s, const RspPacket &p);

} // namespace EmbDebug

#endif
