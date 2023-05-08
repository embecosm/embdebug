
// RSP packet: implementation
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "RspPacket.h"
#include "Utils.h"

using std::cerr;
using std::dec;
using std::endl;
using std::hex;
using std::ostream;
using std::setfill;
using std::setw;

using namespace EmbDebug;

// Define the bufSize with its default value
std::size_t RspPacket::bufSize = 10000;

//! Default constructor
RspPacket::RspPacket() : len(0) {
  data = new char[bufSize];
  ::memset(data, 0, bufSize);
}

//! Copy constructor
RspPacket::RspPacket(const RspPacket &other) {
  data = new char[bufSize];
  ::memcpy(data, other.data, bufSize);
  len = other.len;
}

//! Move constructor
RspPacket::RspPacket(RspPacket &&other) {
  data = std::move(other.data);
  len = other.len;
  other.data = nullptr;
  other.len = 0;
}

//! Constructor from a builder
RspPacket::RspPacket(const RspPacketBuilder &builder) {
  data = new char[bufSize];
  ::memcpy(data, builder.data, bufSize);
  len = builder.len;
}

//! Destructor
RspPacket::~RspPacket() {
  if (data != nullptr)
    delete[] data;
  data = nullptr;
}

//! Create a new packet from a const string as a hex encoded string for qRcmd.

//! The reply to qRcmd packets can be O followed by hex encoded ASCII.

//! @param  str  The string to copy into the data packet before sending
RspPacket RspPacket::CreateHexStr(const char *str) {
  RspPacket response;
  std::size_t slen = strlen(str);

  // Construct the packet to send, so long as string is not too big, otherwise
  // truncate. Add EOS at the end for convenient debug printout
  if (slen >= (bufSize / 2 - 1)) {
    cerr << "Warning: String \"" << str
         << "\" too large for RSP packet: truncated\n"
         << endl;
    slen = bufSize / 2 - 1;
  }

  // Construct the string the hard way
  response.data[0] = 'O';
  for (std::size_t i = 0; i < slen; i++) {
    int nybble_hi = str[i] >> 4;
    int nybble_lo = str[i] & 0x0f;

    response.data[i * 2 + 1] = nybble_hi + (nybble_hi > 9 ? 'a' - 10 : '0');
    response.data[i * 2 + 2] = nybble_lo + (nybble_lo > 9 ? 'a' - 10 : '0');
  }
  response.len = slen * 2 + 1;
  response.data[response.len] = 0;

  return response;
}

//! Create a packet from a const string as a hex encoded string for qRcmd.

//! The reply to qRcmd packets can be O followed by hex encoded ASCII and the
//! client will print them on standard output. If there is no initial O, then
//! the code is silently put into a buffer by the client.

//! @param  str        The string to copy into the data packet before sending
//! @param  toStdoutP  TRUE if the client should send to stdout, FALSE if the
//!                    result should silently go into a buffer.
RspPacket RspPacket::CreateRcmdStr(const char *str, const bool toStdoutP) {
  RspPacket response;
  std::size_t slen = strlen(str);

  // Construct the packet to send, so long as string is not too big, otherwise
  // truncate. Add EOS at the end for convenient debug printout
  if (slen >= (bufSize / 2 - 1)) {
    cerr << "Warning: String \"" << str
         << "\" too large for RSP packet: truncated\n"
         << endl;
    slen = bufSize / 2 - 1;
  }

  // Construct the string the hard way
  int offset;
  if (toStdoutP) {
    response.data[0] = 'O';
    offset = 1;
  } else {
    offset = 0;
  }

  for (unsigned int i = 0; i < slen; i++) {
    uint8_t nybble_hi = str[i] >> 4;
    uint8_t nybble_lo = str[i] & 0x0f;

    response.data[i * 2 + offset + 0] =
        static_cast<char>(nybble_hi + (nybble_hi > 9 ? 'a' - 10 : '0'));
    response.data[i * 2 + offset + 1] =
        static_cast<char>(nybble_lo + (nybble_lo > 9 ? 'a' - 10 : '0'));
  }
  response.len = slen * 2 + offset;
  response.data[response.len] = 0;

  return response;
}

// Move operator
RspPacket &RspPacket::operator=(RspPacket &&other) {
  if (data != nullptr)
    delete[] data;
  data = std::move(other.data);
  len = other.len;
  other.data = nullptr;
  other.len = 0;
  return *this;
}

//! Create a packet from a printf-style call

//! @return a packet with the printf-formatted string
RspPacket RspPacket::CreateFormatted(const char *format, ...) {
  RspPacket response;
  va_list args;
  va_start(args, format);
  response.len = vsnprintf(response.data, 10000, format, args);
  va_end(args);
  return response;
}

//! Default constructor to allocate data buffer
RspPacketBuilder::RspPacketBuilder() {
  data = new char[RspPacket::getMaxPacketSize()];
  ::memset(data, 0, RspPacket::getMaxPacketSize());
}

//! Default constructor to free data buffer
RspPacketBuilder::~RspPacketBuilder() {
  if (data != nullptr)
    delete[] data;
  data = nullptr;
}

//! Add a C string to the current packet
RspPacketBuilder &RspPacketBuilder::operator+=(const char *str) {
  std::size_t _len = ::strlen(str);
  addData(str, _len);
  return *this;
}

//! Add a char to the current packet
RspPacketBuilder &RspPacketBuilder::operator+=(const char c) {
  if (len == RspPacket::getMaxPacketSize()) {
    std::cerr << "Warning: RspPacketBuilder length exceeded, ignoring "
              << EMBDEBUG_PRETTY_FUNCTION << std::endl;
    return *this;
  }
  data[len] = c;
  len++;
  return *this;
}

//! Add a C string to the current packet
void RspPacketBuilder::addData(const char *str) {
  std::size_t _len = ::strlen(str);
  addData(str, _len);
}

//! Add a byte buffer to the current packet
void RspPacketBuilder::addData(const char *str, std::size_t _len) {
  if ((len + _len) > RspPacket::getMaxPacketSize()) {
    std::cerr << "Warning: RspPacketBuilder length exceeded, ignoring "
              << EMBDEBUG_PRETTY_FUNCTION << std::endl;
    return;
  }
  ::memcpy(&data[len], str, _len);
  len += _len;
}

namespace EmbDebug {

//! Output stream operator

//! @param[out] s  Stream to output to
//! @param[in]  p  Packet to output
ostream &operator<<(ostream &s, const RspPacket &p) {
  return s << "RSP packet: " << std::dec << std::setw(3) << p.getLen()
           << std::setw(0) << " chars, \"" << p.getRawData() << "\"";
}

} // namespace EmbDebug
