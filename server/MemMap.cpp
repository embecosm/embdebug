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

//! Constructor

//! Placeholder for now.

MemMap::MemMap() {}

//! Destructor

//! Placeholder for now.

MemMap::~MemMap() {}

void MemMap::addRegion(const uint64_t base, const uint64_t start,
                       const uint64_t end, const MemMap::Type type) {

  if ((base & start) != 0 || (base & end) != 0) {
    std::cerr << "Warning: Base and start/end of region overlap" << std::endl;
  }

  MemMapEntry m = {base | start, base | end, type};

  mMemMap.push_back(m);
}

//! Helper function to identify address space

//! In the event of an error, a warning is printed.

//! @param[in]  addr    Address to access
//! @param[in]  size    Number of bytes to access
//! @return  The memory type, which will be UNKNOWN if there was an error.

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

  if (size > 0)
    for (auto it = mMemMap.begin(); it != mMemMap.end(); it++) {
      const uint32_t locAddr = (addr + size - 1);

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
