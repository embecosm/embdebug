// Process/thread ID implementation: declaration

// Copyright (C) 2009, 2013, 2017  Embecosm Limited <info@embecosm.com>

// Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
// Contributor Ian Bolton <ian.bolton@embecosm.com>

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

#ifndef PTID_H
#define PTID_H

#include <iostream>

namespace EmbDebug {

//! Module representing a PTID.

class Ptid {
public:
  // Constructor and destructor

  Ptid(const int pid, const int tid);
  ~Ptid();

  // Accessors

  void pid(const int _pid);
  int pid() const;
  void tid(const int _tid);
  int tid() const;

  // I/O functions

  bool decode(const char *buf);
  bool encode(char *buf);
  bool crystalize(const int defaultPid, const int defaultTid);
  bool validate();

  // Useful constants

  static const int PTID_INV = -2; //!< Invalid (extension to standard)
  static const int PTID_ALL = -1; //!< All processes/threads
  static const int PTID_ANY = 0;  //!< Any process/thread

private:
  int mPid; //!< Process ID
  int mTid; //!< Thread ID

  // stream operators have to be friends to access private members

  friend std::ostream &operator<<(std::ostream &s, Ptid p);

  // Make the default constructor private so it cannot be used

  Ptid(){};

  // Helper functions

  int decodeField(const char *buf, const std::size_t len) const;
  std::size_t encodeField(char *buf, int ptid);

}; // Ptid ()

} // namespace EmbDebug

#endif // PTID_H
