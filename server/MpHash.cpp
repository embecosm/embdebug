// Matchpoint hash table: definition
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include <cstdlib>

#include "MpHash.h"

using namespace EmbDebug;

//! Constructor

//! Allocate the hash table
//! @param[in] size  Number of slots in the  hash table. Defaults to
//!                  DEFAULT_MP_HASH_SIZE.
MpHash::MpHash(int _size) : size(_size) {
  // Allocate and clear the hash table
  hashTab = new MpEntry *[size];

  for (int i = 0; i < size; i++) {
    hashTab[i] = NULL;
  }
} // MpHash ()

//! Destructor

//! Free the hash table
MpHash::~MpHash() { delete[] hashTab; } // ~MpHash ()

//! Add an entry to the hash table

//! Add the entry if it wasn't already there. If it was there do nothing. The
//! match just be on type and addr. The instr need not match, since if this is
//! a duplicate insertion (perhaps due to a lost packet) they will be
//! different.

//! @note This method allocates memory. Care must be taken to delete it when
//! done.

//! @param[in] type   The type of matchpoint
//! @param[in] addr   The address of the matchpoint
//! @para[in]  instr  The instruction to associate with the address
void MpHash::add(MpType type, uint32_t addr, uint32_t instr) {
  int hv = addr % size;
  MpEntry *curr;

  // See if we already have the entry
  for (curr = hashTab[hv]; NULL != curr; curr = curr->next) {
    if ((type == curr->type) && (addr == curr->addr)) {
      return; // We already have the entry
    }
  }

  // Insert the new entry at the head of the chain
  curr = new MpEntry();

  curr->type = type;
  curr->addr = addr;
  curr->instr = instr;
  curr->next = hashTab[hv];

  hashTab[hv] = curr;

} // add ()

//! Look up an entry in the matchpoint hash table

//! The match must be on type AND addr.

//! @param[in] type   The type of matchpoint
//! @param[in] addr   The address of the matchpoint

//! @return  The entry found, or NULL if the entry was not found
MpEntry *MpHash::lookup(MpType type, uint32_t addr) {
  int hv = addr % size;

  // Search
  for (MpEntry *curr = hashTab[hv]; NULL != curr; curr = curr->next) {
    if ((type == curr->type) && (addr == curr->addr)) {
      return curr; // The entry found
    }
  }

  return NULL; // Not found

} // lookup ()

//! Delete an entry from the matchpoint hash table

//! If it is there the entry is deleted from the hash table. If it is not
//! there, no action is taken. The match must be on type AND addr. The entry
//! (MpEntry::) is itself deleted

//! The usual fun and games tracking the previous entry, so we can delete
//! things.

//! @param[in]  type   The type of matchpoint
//! @param[in]  addr   The address of the matchpoint
//! @param[out] instr  Location to place the instruction found. If NULL (the
//!                    default) then the instruction is not written back.

//! @return  TRUE if an entry was found and deleted
bool MpHash::remove(MpType type, uint32_t addr, uint32_t *instr) {
  int hv = addr % size;
  MpEntry *prev = NULL;
  MpEntry *curr;

  // Search
  for (curr = hashTab[hv]; NULL != curr; curr = curr->next) {
    if ((type == curr->type) && (addr == curr->addr)) {
      // Found - delete. Method depends on whether we are the head of
      // chain.
      if (NULL == prev) {
        hashTab[hv] = curr->next;
      } else {
        prev->next = curr->next;
      }

      if (NULL != instr) {
        *instr = curr->instr; // Return the found instruction
      }

      delete curr;
      return true; // Success
    }

    prev = curr;
  }

  return false; // Not found

} // remove ()
