// GDB Server Utilties: definition
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef EMBDEBUG_UTILS_H
#define EMBDEBUG_UTILS_H

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
  static void regVal2Hex(uint64_t val, char *buf, std::size_t numBytes,
                         bool isLittleEndianP);
  static uint64_t hex2RegVal(const char *buf, std::size_t numBytes,
                             bool isLittleEndianP);
  static std::size_t val2Hex(uint64_t val, char *buf);
  static uint64_t hex2Val(const char *buf, std::size_t len);
  static void ascii2Hex(char *dest, const char *src);
  static void hex2Ascii(char *dest, const char *src);
  static std::size_t rspUnescape(char *buf, std::size_t len);
  static std::vector<std::string> &split(const std::string &s,
                                         const std::string &delim,
                                         std::vector<std::string> &elems);

  static bool str2int(int &i, const std::string &str, int base = 0);

private:
  // Private constructor cannot be instantiated

  Utils(){};
};

} // namespace EmbDebug

#endif
