// ByteView
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef EMBDEBUG_BYTEVIEW_H
#define EMBDEBUG_BYTEVIEW_H

#include <cassert>
#include <cstddef>
#include <cstring>
#include <limits>

namespace EmbDebug {

//! A lightweight wrapper around a C str[] that allows simple comparisons
//! Note: It does not own the memory it points to.
class ByteView {
  // Pointer and length
  const char *start;
  std::size_t len;

public:
  static const std::size_t n_pos = std::numeric_limits<std::size_t>::max();

  ByteView() : start(nullptr), len(0) {}
  ByteView(const char *_start) : start(_start) { len = ::strlen(_start); }
  ByteView(const char *_start, std::size_t _len) : start(_start), len(_len) {}
  bool operator==(const ByteView &other) const {
    return other.len == len && 0 == ::memcmp(start, other.start, other.len);
  }
  bool operator!=(const ByteView &other) const { return !(*this == other); }
  bool operator==(const char *other) const {
    return ::strlen(other) == len && 0 == ::memcmp(start, other, len);
  }
  bool operator!=(const char *other) const { return !(*this == other); }
  char operator[](std::size_t index) const {
    assert(index < len && "Attempted to access ByteView out of bounds");
    return start[index];
  }

  const char *getData() const { return start; }
  std::size_t getLen() const { return len; }

  bool starts_with(const char *prefix) {
    return len >= ::strlen(prefix) &&
           0 == ::memcmp(start, prefix, ::strlen(prefix));
  }

  ByteView first(std::size_t offset) const {
    if (offset > len)
      return *this;
    return ByteView(start, offset);
  }

  ByteView lstrip(std::size_t offset) const {
    if (offset > len)
      return ByteView();
    return ByteView(start + offset, len - offset);
  }

  std::size_t find(char c) const {
    std::size_t offset = 0;
    while (offset < len) {
      if (start[offset] == c)
        return offset;
      offset++;
    }
    return n_pos;
  }

  bool fromHex(uint64_t &value) const {
    value = 0;
    for (std::size_t i = 0; i < len; i++) {
      value *= 16;
      char nibble = start[i];
      if (nibble >= '0' && nibble <= '9')
        value += (nibble - '0');
      else if (nibble >= 'a' && nibble <= 'f')
        value += 10 + (nibble - 'a');
      else if (nibble >= 'A' && nibble <= 'F')
        value += 10 + (nibble - 'A');
      else
        return false;
    }
    return true;
  }
};
} // namespace EmbDebug

#endif
