// GDB Server Utilties: definition

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
