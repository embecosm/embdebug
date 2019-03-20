// Class to handle decoding vCont packets.

// Copyright (C) 2017  Embecosm Limited <info@embecosm.com>

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

#ifndef VCONTACTIONS_H
#define VCONTACTIONS_H

#include "Ptid.h"

#include <vector>

namespace EmbDebug {

class VContActions {
public:
  // Decode vCont packet in STR.
  VContActions(const char *str) { mValid = parse(str); }

  // Return true if the vCont packet was decoded successfully, otherwise,
  // return false.  Other than the constructor you should not call any
  // other methods on an object of this class unless your know that the
  // object is valid.
  bool valid(void) const { return mValid; }

  // Return true if the vCont packet effected more than one core.
  bool effectsMultipleCores(void) const;

  // Return the action letter 'c', 'C', 's', or 'S' that is applied to core
  // NUM.  If/when we want to support signals in the future this interface
  // will need to be expanded.
  char getCoreAction(unsigned int num) const;

private:
  // Delete alternative constructors.
  VContActions() = delete;
  VContActions(const VContActions &) = delete;

  // Actually parse the vCont packet, return true if the parse was OK,
  // otherwise return false.
  bool parse(const char *str);

  // Is this object valid.
  bool mValid;

  // The list of actions extracted from the vCont packet.  This is a pretty
  // crude storage format, which we should probably improve on.
  std::vector<std::pair<std::string, Ptid>> mActions;
};

} // namespace EmbDebug

#endif // VCONTACTIONS_H
