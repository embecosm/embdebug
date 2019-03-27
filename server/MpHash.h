// Matchpoint hash table: definition
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef MP_HASH_H
#define MP_HASH_H

#include <stdint.h>

namespace EmbDebug {

//! Default size of the matchpoint hash table. Largest prime < 2^10
#define DEFAULT_MP_HASH_SIZE 1021

//! Enumeration of different types of matchpoint.

//! These have explicit values matching the second digit of 'z' and 'Z'
//! packets.
enum MpType {
  BP_MEMORY = 0,
  BP_HARDWARE = 1,
  WP_WRITE = 2,
  WP_READ = 3,
  WP_ACCESS = 4
};

class MpHash;

//! A structure for a matchpoint hash table entry
struct MpEntry {
public:
  friend class MpHash; // The only one which can get at next

  MpType type;    //!< Type of matchpoint
  uint32_t addr;  //!< Address with the matchpoint
  uint32_t instr; //!< Substituted instruction

private:
  MpEntry *next; //!< Next in this slot
};

//! A hash table for matchpoints

//! We do this as our own open hash table. Our keys are a pair of entities
//! (address and type), so STL map is not trivial to use.

class MpHash {
public:
  // Constructor and destructor
  MpHash(int _size = DEFAULT_MP_HASH_SIZE);
  ~MpHash();

  // Accessor methods
  void add(MpType type, uint32_t addr, uint32_t instr);
  MpEntry *lookup(MpType type, uint32_t addr);
  bool remove(MpType type, uint32_t addr, uint32_t *instr = NULL);

private:
  //! The hash table
  MpEntry **hashTab;

  //! Size of the hash table
  int size;
};

} // namespace EmbDebug

#endif
