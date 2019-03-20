// RSP packet: definition

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

#ifndef RSP_PACKET_H
#define RSP_PACKET_H

#include <iostream>

//! Class for RSP packets

//! Can't be null terminated, since it may include zero bytes
class RspPacket {
public:
  //! The data buffer. Allow direct access to avoid unnecessary copying.
  char *data;

  // Constructor and destructor
  RspPacket(int _bufSize);
  ~RspPacket();

  // Pack a constant string into a packet
  void packStr(const char *str);    // For fixed packets
  void packRcmdStr(const char *str, // For qRcmd replies
                   const bool toStdoutP);

  // Pack a hex encoded string into a packet
  void packHexstr(const char *str);

  // Accessors
  int getBufSize();
  int getLen();
  void setLen(int _len);

private:
  //! The data buffer size
  std::size_t bufSize;

  //! Number of chars in the data buffer (<= bufSize)
  std::size_t len;
};

//! Stream output
std::ostream &operator<<(std::ostream &s, RspPacket &p);

#endif // RSP_PACKET_SC_H
