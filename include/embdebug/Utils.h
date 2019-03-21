// GDB Server Utilties: definition
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <string>
#include <vector>

namespace EmbDebug {

//! A class offering a number of convenience utilities for the GDB Server.

//! All static functions. This class is not intended to be instantiated.
class Utils {
public:
  static bool isHexStr(const char *buf, const std::size_t len);
  static uint8_t char2Hex(int c);
  static char hex2Char(uint8_t d);
  static void regVal2Hex(uint64_t val, char *buf, int numBytes,
                         bool isLittleEndianP);
  static uint64_t hex2RegVal(const char *buf, int numBytes,
                             bool isLittleEndianP);
  static std::size_t val2Hex(uint64_t val, char *buf);
  static uint64_t hex2Val(const char *buf, std::size_t len);
  static void ascii2Hex(char *dest, char *src);
  static void hex2Ascii(char *dest, char *src);
  static int rspUnescape(char *buf, int len);
  static std::vector<std::string> &split(const std::string &s,
                                         const std::string &delim,
                                         std::vector<std::string> &elems);

  static bool str2int(int &i, const std::string &str, int base = 0);

private:
  // Private constructor cannot be instantiated

  Utils(){};

}; // class Utils

} // namespace EmbDebug

#endif // UTILS_H
