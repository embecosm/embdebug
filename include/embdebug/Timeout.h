// GDB Timeout: declaration
//
// } // namespace EmbDebug

// Copyright (C) 2009, 2013, 2017  Embecosm Limited <info@embecosm.com>

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

#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <chrono>
#include <cstdint>

#include "ITarget.h"

namespace EmbDebug {

//! Class representing a timeout in the GDB server.

//! We may wish to represent timeouts as a cycle count or as a wall clock
//! time. The former is reproducible, but only feasible with models.

//! This class allows a timeout to be either a real timeout, or a cycle count
//! timeout, but not both. Which it is depends on the constructor or set
//! accessor used. This provides flexibility in switching between timeout
//! types.

//! We also may have no timeout set.

class Timeout {
public:
  // Constructors and destructor.

  Timeout();
  Timeout(const std::chrono::duration<double> realTimeout);
  Timeout(const uint64_t cycleTimeout);
  ~Timeout();

  // Accessors

  void clearTimeout();
  std::chrono::duration<double> realTimeout() const;
  void realTimeout(const std::chrono::duration<double> realTimeout);
  uint64_t cycleTimeout() const;
  void cycleTimeout(const uint64_t cycleTimeout);
  bool haveTimeout() const;
  bool isRealTimeout() const;
  bool isCycleTimeout() const;

  // Handle time stamps

  void timeStamp(ITarget *cpu);
  bool timedOut(ITarget *cpu) const;

private:
  //! An enumeration for the timeout type.

  enum class Type {
    NONE,  //!< No timeout.
    REAL,  //!< Wall clock timeout.
    CYCLE, //!< Cycle count timeout.
  };

  //! Enum to indicate which timeout, if any

  Type mTimeoutType;

  //! Real timeout

  std::chrono::duration<double> mRealTimeout;

  //! Cycle count timeout

  uint64_t mCycleTimeout;

  //! Real time stamp

  std::chrono::time_point<std::chrono::system_clock,
                          std::chrono::duration<double>>
      mRealStamp;

  //! Cycle count time stamp

  uint64_t mCycleStamp;

}; // Timeout ()

} // namespace EmbDebug

#endif // TIMEOUT_H
