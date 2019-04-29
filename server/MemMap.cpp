// GDB RSP server memory map: definition
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2017-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include <iostream>

#include "embdebug/MemMap.h"

using namespace EmbDebug;

MemMap::MemMap() {}
MemMap::~MemMap() {}

void MemMap::addRegion(const uint64_t base, const uint64_t start,
                       const uint64_t end, const MemMap::Type type) {

  if ((base & start) != 0 || (base & end) != 0) {
    std::cerr << "Warning: Base and start/end of region overlap" << std::endl;
  }

  MemMapEntry m = {base | start, base | end, type};

  // FIXME: Check to ensure that an added region does not overlap an already
  // existing region.

  mMemMap.push_back(m);
}

MemMap::Type MemMap::findRegion(const uint64_t addr,
                                const std::size_t size) const {
  Type startType = Type::UNKNOWN;
  Type endType = Type::UNKNOWN;

  for (auto it = mMemMap.begin(); it != mMemMap.end(); it++) {
    if ((it->start <= addr) && (addr <= it->end)) {
      startType = it->type;
      break;
    }
  }

  // FIXME: Should it be required that the start and end are in the same
  // region? If not, it would be prudent to at least check that all addresses
  // between the start and end are covered by a contiguous set of regions of a
  // uniform type.

  if (size > 0)
    for (auto it = mMemMap.begin(); it != mMemMap.end(); it++) {
      const uint64_t locAddr = (addr + size - 1);

      if ((it->start <= locAddr) && (locAddr <= it->end)) {
        endType = it->type;
        break;
      }
    }

  if (startType == Type::UNKNOWN) {
    std::cerr << "Warning: Start of memory access 0x" << std::hex << addr
              << std::dec << " not in memory region" << std::endl;
    return Type::UNKNOWN;
  }

  if (size > 0) {
    if (endType == Type::UNKNOWN) {
      std::cerr << "Warning: End of memory access 0x" << std::hex
                << addr + size - 1 << std::dec << " not in memory region"
                << std::endl;
      return Type::UNKNOWN;
    }

    if (startType != endType) {
      std::cerr << "Warning: Memory access 0x" << std::hex << addr << " - 0x"
                << addr + size - 1 << std::dec << " straddles memory regions"
                << std::endl;
      return Type::UNKNOWN;
    }
  }

  return startType;
}
