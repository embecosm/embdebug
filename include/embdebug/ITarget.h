// Generic GDB RSP server interface: declaration

// Copyright (C) 2008-2017  Embecosm Limited <www.embecosm.com>

// Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

// This file is part of the RISC-V GDB server

// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef ITARGET_H
#define ITARGET_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

#include "RegisterSizes.h"

// Classes to which we refer.

class TraceFlags;

//! Generic interface class for GDB RSP server targets.

//! Being an interface class, it has no state.  But it does have some
//! behavior, in the form of stream handling methods.

class ITarget {
public:
  //! Type of resume to use.

  //! STOP is to request the target to stop.  This is for use after a previous
  //! call has indicated a timeout, for targets where stopping is very
  //! expensive.

  enum class ResumeType : int { STEP = 0, CONTINUE = 1, NONE = 2 };

  //! Result of execution methods.

  enum class ResumeRes : uint32_t {
    NONE = 0,        //!< Place holder when we don't want to stop.
    SUCCESS = 1,     //!< Execution was successful.
    FAILURE = 2,     //!< Execution failed.
    INTERRUPTED = 3, //!< Execution interrupted (e.g. breakpoint).
    TIMEOUT = 4,     //!< Execution hit time limit.
    SYSCALL = 5,     //!< Target needs some host I/O.
    STEPPED = 6,     //!< Single step was completed.
    LOCKSTEP = 7,    //!< Lockstep divergence detected.
  };

  //! Type of reset
  enum class ResetType {
    COLD, //!< Equivalent to complete class recreation
    WARM  //!< Set relevant state back to defaul
  };

  //! Type of matchpoint, with mapping to RSP Z/z packet values.

  enum class MatchType : int {
    BREAK = 0,       //!< Software/memory breakpoint
    BREAK_HW = 1,    //!< Hardware breakpoint
    WATCH_WRITE = 2, //!< Write watchpoint
    WATCH_READ = 3,  //!< Read watchpoint
    WATCH_ACCESS = 4 //!< Access watchpoint
  };

  //! Type returned from the wait call.

  enum class WaitRes : int {
    EVENT_OCCURRED = 0, //!< Something interesting happened
    ERROR = 1,          //!< Something went wrong
    TIMEOUT = 2,        //!< Timeout, check for interrupts
  };

  //! Constant that can be used by multi-cpu targets to indicate that no
  //! valid cpu is currently selected.  This number is guaranteed not to
  //! match any valid cpu number.

  static const unsigned int INVALID_CPU_NUMBER = (unsigned int)-1;

  // Virtual destructor has implementation for defined behavior

  explicit ITarget(const TraceFlags *traceFlags __attribute__((unused))){};
  virtual ~ITarget(){};

  virtual ResumeRes terminate() = 0;
  virtual ResumeRes reset(ResetType type) = 0;

  virtual uint64_t getCycleCount() const = 0;
  virtual uint64_t getInstrCount() const = 0;

  // Read contents of a target register.

  virtual std::size_t readRegister(const int reg, uint_reg_t &value) = 0;

  // Write data to a target register.

  virtual std::size_t writeRegister(const int reg, const uint_reg_t value) = 0;

  // Read data from memory.

  virtual std::size_t read(const uint32_t addr, uint8_t *buffer,
                           const std::size_t size) = 0;

  // Write data to memory.

  virtual std::size_t write(const uint32_t addr, const uint8_t *buffer,
                            const std::size_t size) = 0;

  // Insert and remove a matchpoint (breakpoint or watchpoint) at the given
  // address.  Return value indicates whether the operation was successful.

  virtual bool insertMatchpoint(const uint32_t addr,
                                const MatchType matchType) = 0;
  virtual bool removeMatchpoint(const uint32_t addr,
                                const MatchType matchType) = 0;

  // Generic pass through of command

  virtual bool command(const std::string cmd, std::ostream &stream) = 0;

  // Verilator support

  virtual double timeStamp() = 0;

  // How many cpus are there?  Will return a value greater than or equal to
  // one.

  virtual unsigned int getCpuCount(void) = 0;

  // Which cpu is the "current" cpu?  This controls where reads and writes
  // will go.  Will always return a value greater than or equal to zero,
  // and less than the value returned by getCpuCount.

  virtual unsigned int getCurrentCpu(void) = 0;

  // Set the "current" cpu.  This controls where reads and writes will go,
  // which registers will be accessed, etc.  The value passed must be
  // greater than or equal to zero, and less than the value returned by
  // getCpuCount.  Behaviour for an invalid value is undefined.

  virtual void setCurrentCpu(unsigned int) = 0;

  // Prepare each core to perform actions within the vector.
  virtual bool prepare(const std::vector<ITarget::ResumeType> &) = 0;

  // Move core(s) that are going to do something into a running state.
  virtual bool resume(void) = 0;

  // Wait for some stop event to occur on a core.
  virtual WaitRes wait(std::vector<ResumeRes> &) = 0;

  // Tell all running cores to halt.
  virtual bool halt(void) = 0;

private:
  // Don't allow the default constructors

  ITarget(){};
  ITarget(const ITarget &){};

}; // class ITarget

// Output operators for scoped enumerations

std::ostream &operator<<(std::ostream &s, ITarget::ResumeType p);
std::ostream &operator<<(std::ostream &s, ITarget::ResumeRes p);
std::ostream &operator<<(std::ostream &s, ITarget::MatchType p);

#endif // ITARGET_H
