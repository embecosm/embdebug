// GDB RSP server memory map: declaration

// Copyright (C) 2017  Embecosm Limited <info@embecosm.com>

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

#ifndef MEMMAP_H
#define MEMMAP_H

#include <cstdint>
#include <vector>


//! A memory map class

//! A convenience class for describing memory maps

class MemMap final
{
 public:

  //! Enumeration for memory map.

  enum class Type
  {
    UNKNOWN,			//!< Unknown memory type
    IMEM,			//!< Instruction memory
    DMEM,			//!< Data memory
    PERS,			//!< peripheral space
    ALIAS,			//!< alias space
    EMEM,			//!< ethernet memory
    PCIMEM			//!< PCI memory space
  };

  // Constructor and destructor.

  MemMap ();
  ~MemMap ();

  // Public methods

  void  addRegion (const uint64_t  base,
		   const uint64_t  start,
		   const uint64_t  end,
		   const Type      type);
  Type  findRegion (const uint64_t     addr,
		    const std::size_t  size) const;

 private:

  struct MemMapEntry
  {
    uint64_t  start;
    uint64_t  end;
    Type      type;
  };

  //! The memory map

  std::vector <MemMapEntry> mMemMap;

  // No public default copy constructor.

  MemMap (const MemMap &) {};

};	// class MemMap

#endif	// MEMMAP_H


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
