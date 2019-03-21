// GDB RSP server memory map: declaration
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2017-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef MEMMAP_H
#define MEMMAP_H

#include <cstdint>
#include <vector>

namespace EmbDebug {

//! A memory map class

//! A convenience class for describing memory maps

class MemMap final {
public:
  //! Enumeration for memory map.

  enum class Type {
    UNKNOWN, //!< Unknown memory type
    IMEM,    //!< Instruction memory
    DMEM,    //!< Data memory
    PERS,    //!< peripheral space
    ALIAS,   //!< alias space
    EMEM,    //!< ethernet memory
    PCIMEM   //!< PCI memory space
  };

  // Constructor and destructor.

  MemMap();
  ~MemMap();

  // Public methods

  void addRegion(const uint64_t base, const uint64_t start, const uint64_t end,
                 const Type type);
  Type findRegion(const uint64_t addr, const std::size_t size) const;

private:
  struct MemMapEntry {
    uint64_t start;
    uint64_t end;
    Type type;
  };

  //! The memory map

  std::vector<MemMapEntry> mMemMap;

  // No public default copy constructor.

  MemMap(const MemMap &){};

}; // class MemMap

} // namespace EmbDebug

#endif // MEMMAP_H
