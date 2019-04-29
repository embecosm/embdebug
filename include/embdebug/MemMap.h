// GDB RSP server memory map: declaration
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2017-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef EMBDEBUG_MEMMAP_H
#define EMBDEBUG_MEMMAP_H

#include <cstdint>
#include <vector>

namespace EmbDebug {

//! \brief Convenience class for describing memory layout
class MemMap final {
public:
  //! Types of memory that region represents
  enum class Type {
    UNKNOWN, //!< Unknown memory type
    IMEM,    //!< Instruction memory
    DMEM,    //!< Data memory
    PERS,    //!< Peripheral space
    ALIAS,   //!< Alias space
    EMEM,    //!< Ethernet memory
    PCIMEM   //!< PCI memory space
  };

  MemMap();
  ~MemMap();
  MemMap(const MemMap &) = delete;

  //! \brief Add a new region to the memory map
  //!
  //! The base address bits should not overlap the start or end addresses.
  //! A region should not overlap another region already in the map.
  //!
  //! \param[in] base  The base address of the region.
  //! \param[in] start The start address of the region.
  //! \param[in] end   The end address of the region.
  //! \param[in] type  Type of the memory region
  void addRegion(const uint64_t base, const uint64_t start, const uint64_t end,
                 const Type type);

  //! \brief Helper function identifying the type of memory of an address
  //!
  //! In the event of an error, a warning is printed.
  //!
  //! \param[in] addr Start of the memory whose type is to be determined
  //! \param[in] size The size of the memory block, this may be zero.
  //! \return The type of the range of memory, or UNKNOWN if there was an error.
  //!         An error may occur if the address does not belong to a memory
  //!         region, or if the memory range straddles multiple regions of
  //!         different types.
  Type findRegion(const uint64_t addr, const std::size_t size) const;

private:
  struct MemMapEntry {
    uint64_t start;
    uint64_t end;
    Type type;
  };

  std::vector<MemMapEntry> mMemMap;
};

} // namespace EmbDebug

#endif
