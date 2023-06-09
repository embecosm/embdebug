// Remote Serial Protocol connection: implementation
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2017-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include <iomanip>
#include <iostream>

#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstring>

#include "AbstractConnection.h"
#include "Utils.h"

using std::cerr;
using std::cout;
using std::dec;
using std::endl;
using std::flush;
using std::hex;
using std::setfill;
using std::setw;

using namespace EmbDebug;

//! Get the next packet from the RSP connection

//! Modeled on the stub version supplied with GDB. This allows the user to
//! replace the character read function, which is why we get stuff a character
//! at at time.

//! Unlike the reference implementation, we don't deal with sequence
//! numbers. GDB has never used them, and this implementation is only intended
//! for use with GDB 6.8 or later. Sequence numbers were removed from the RSP
//! standard at GDB 5.0.

//! @param[in] pkt  The packet for storing the result.

//! @return  bool:      TRUE to indicate success, FALSE otherwise (means a
//!                     communications failure)
//!          RspPacket: Valid packet if first return value is TRUE, empty packet
//!                     otherwise
std::pair<bool, RspPacket> AbstractConnection::getPkt() {
  RspPacketBuilder newPkt;

  // Keep getting packets, until one is found with a valid checksum
  while (true) {
    unsigned char checksum; // The checksum we have computed
    int ch;                 // Current character

    // Wait around for the start character ('$'). Ignore all other
    // characters
    ch = getRspChar();
    while (ch != '$') {
      if (-1 == ch) {
        return {false, RspPacket()}; // Connection failed
      } else {
        ch = getRspChar();
      }
    }

    // Read until a '#' or end of buffer is found
    checksum = 0;
    while (newPkt.getRemaining()) {
      ch = getRspChar();

      if (-1 == ch) {
        return {false, RspPacket()}; // Connection failed
      }

      // If we hit a start of line char begin all over again
      if ('$' == ch) {
        checksum = 0;
        newPkt.erase();

        continue;
      }

      // Break out if we get the end of line char
      if ('#' == ch) {
        break;
      }

      // Update the checksum and add the char to the buffer
      checksum = checksum + (unsigned char)ch;
      newPkt += (char)ch;
    }

    // If we have a valid end of packet char, validate the checksum. If we
    // don't it's because we ran out of buffer in the previous loop.
    if ('#' == ch) {
      unsigned char xmitcsum; // The checksum in the packet

      ch = getRspChar();
      if (-1 == ch) {
        return {false, RspPacket()}; // Connection failed
      }
      assert(Utils::isHexStr((char *)&ch, 1));
      xmitcsum = Utils::char2Hex(ch) << 4;

      ch = getRspChar();
      if (-1 == ch) {
        return {false, RspPacket()}; // Connection failed
      }

      assert(Utils::isHexStr((char *)&ch, 1));
      xmitcsum += Utils::char2Hex(ch);

      // If the checksums don't match print a warning, and put the
      // negative ack back to the client. Otherwise put a positive ack.
      if (mNoAckMode) {
        RspPacket pkt(newPkt);
        if (traceFlags->traceRsp()) {
          cout << "RSP trace: getPkt: " << pkt << endl;
        }
        return {true, pkt};
      }
      if (checksum != xmitcsum) {
        cerr << "Warning: Bad RSP checksum: Computed 0x" << setw(2)
             << setfill('0') << hex << checksum << ", received 0x" << xmitcsum
             << setfill(' ') << dec << endl;
        if (!putRspChar('-')) // Failed checksum
        {
          return {false, RspPacket()}; // Comms failure
        }
      } else {
        if (!putRspChar('+')) // successful transfer
        {
          return {false, RspPacket()}; // Comms failure
        } else {
          RspPacket pkt(newPkt);
          if (traceFlags->traceRsp()) {
            cout << "RSP trace: getPkt: " << pkt << endl;
          }

          return {true, pkt}; // Success
        }
      }
    } else {
      cerr << "Warning: RSP packet overran buffer" << endl;
    }
  }
}

//! Put the packet out on the RSP connection

//! Modeled on the stub version supplied with GDB. Put out the data preceded
//! by a '$', followed by a '#' and a one byte checksum. '$', '#', '*' and '}'
//! are escaped by preceding them with '}' and then XORing the character with
//! 0x20.

//! @param[in] pkt  The Packet to transmit

//! @return  TRUE to indicate success, FALSE otherwise (means a communications
//!          failure).
bool AbstractConnection::putPkt(const RspPacket &pkt) {
  std::size_t len = pkt.getLen();
  int ch; // Ack char

  // Construct $<packet info>#<checksum>. Repeat until the GDB client
  // acknowledges satisfactory receipt.
  do {
    unsigned char checksum = 0; // Computed checksum
    std::size_t count = 0;      // Index into the buffer

    if (!putRspChar('$')) // Start char
    {
      return false; // Comms failure
    }

    // Body of the packet
    for (count = 0; count < len; count++) {
      unsigned char ch = pkt.getData()[count];

      // Check for escaped chars
      if (('$' == ch) || ('#' == ch) || ('*' == ch) || ('}' == ch)) {
        ch ^= 0x20;
        checksum += (unsigned char)'}';
        if (!putRspChar('}')) {
          return false; // Comms failure
        }
      }

      checksum += ch;
      if (!putRspChar(ch)) {
        return false; // Comms failure
      }
    }

    if (!putRspChar('#')) // End char
    {
      return false; // Comms failure
    }

    // Computed checksum
    if (!putRspChar(Utils::hex2Char(checksum >> 4))) {
      return false; // Comms failure
    }
    if (!putRspChar(Utils::hex2Char(checksum % 16))) {
      return false; // Comms failure
    }

    // Check for ack of connection failure
    if (mNoAckMode)
      break;
    ch = getRspChar();
    if (-1 == ch) {
      return false; // Comms failure
    } else if (BREAK_CHAR == ch) {
      // Handle a break arriving while we're waiting for a packet ACK.
      // We only support the arrival of a single break, which I think
      // is acceptable.
      mHavePendingBreak = true;
      ch = getRspChar();
      assert(ch != BREAK_CHAR);
    }
  } while ('+' != ch);

  if (traceFlags->traceRsp()) {
    cout << "RSP trace: putPkt: " << pkt << endl;
  }

  return true;
}

//! Put a single character out on the RSP connection

//! Potentially we can have an OS specific implemenation of the underlying
//! routine.

//! @param[in] c  The character to put out
//! @return  TRUE if char sent OK, FALSE if not (communications failure)

bool AbstractConnection::putRspChar(char c) { return putRspCharRaw(c); }

//! Get a single character from the RSP connection with buffering

//! Utility routine for use by other functions.  This is built on the raw
//! read function.

//! This function will first return the possibly buffered character (buffering
//! caused by calling 'haveBreak'.

//! @return  The character received or -1 on failure

int AbstractConnection::getRspChar() {
  int ch;

  if (mNumGetBufChars > 1)
    cerr << "Warning: Too many cached characters (" << dec << mNumGetBufChars
         << ")" << endl;

  if (mNumGetBufChars > 0) {
    ch = mGetCharBuf;
    mNumGetBufChars = 0;
  } else
    // It's tempting to think we can check for BREAK_CHAR here.  DON'T!
    // This method is used when reading in whole packets, and the
    // BREAK_CHAR is only special when it arrives outside of a packet.
    ch = getRspCharRaw(true);

  return ch;
}

//! Have we received a break character.

//! Since we only check fo this between packets, we don't have to worry about
//! being in the middle of a packet.

//! @Note  We only peek, so no character is actually consumed from the input.

//! @return  TRUE if we have received a break character, FALSE otherwise.

bool AbstractConnection::haveBreak() {
  if (!mHavePendingBreak && mNumGetBufChars == 0) {
    // Non-blocking read to possibly get a character.

    int nextChar = getRspCharRaw(false);

    if (nextChar != -1) {
      if (nextChar == BREAK_CHAR)
        mHavePendingBreak = true;
      else {
        mGetCharBuf = nextChar;
        mNumGetBufChars = 1;
      }
    }
  }

  if (mHavePendingBreak) {
    mHavePendingBreak = false;
    return true;
  } else
    return false;
}
