// Generic GDB RSP server interface: declaration
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2008-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef EMBDEBUG_ITARGET_H
#define EMBDEBUG_ITARGET_H

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

#include "Gdbserver_compat.h"
#include "RegisterSizes.h"

namespace EmbDebug {

class TraceFlags;

//! \brief Generic interface for target for the GDB server
//!
//! This is a pure abstract class and is the main interface to be implemented
//! by any targets to be driven by the GDB server.
class ITarget {
public:
  //! The type of action which will be performed when a core is resumed.
  enum class ResumeType : int {
    //! Perform a single instruction step and then stop.
    STEP = 0,

    //! Continue until the core is halted or an exception is triggered.
    CONTINUE = 1,

    //! Do nothing.
    NONE = 2
  };

  //! Result after a core is resumed and has come to a halt
  enum class ResumeRes : uint32_t {
    NONE = 0,        //!< Place holder when we don't want to stop.
    SUCCESS = 1,     //!< Execution was successful.
    FAILURE = 2,     //!< Execution failed.
    INTERRUPTED = 3, //!< Execution interrupted (e.g. breakpoint).
    TIMEOUT = 4,     //!< Execution hit time limit.
    SYSCALL = 5,     //!< Execution hit a syscall.
    STEPPED = 6,     //!< A single step was completed.
    LOCKSTEP = 7,    //!< Lockstep divergence was detected.
  };

  //! Type of reset
  enum class ResetType {
    COLD, //!< Equivalent to complete class recreation
    WARM  //!< Set relevant state back to default
  };

  //! The type of a matchpoint, with mappings to RSP Z/z packet values.
  enum class MatchType : int {
    BREAK = 0,       //!< Software/memory breakpoint
    BREAK_HW = 1,    //!< Hardware breakpoint
    WATCH_WRITE = 2, //!< Write watchpoint
    WATCH_READ = 3,  //!< Read watchpoint
    WATCH_ACCESS = 4 //!< Access watchpoint
  };

  //! The result of a wait operation.
  enum class WaitRes : int {
    //! Something interesting happened, such as a breakpoint or system call
    //! was hit by a running core.
    EVENT_OCCURRED = 0,

    //! An error occurred. This is used when the target is in an unrecoverable
    //! state.
    ERROR = 1,

    //! Timeout, relieve control to the GDB server to check for client
    //! interrupts.
    TIMEOUT = 2,
  };

  //! \brief Constant that can be used by multi-cpu targets to indicate that no
  //! valid cpu is currently selected.
  //!
  //! This number is guaranteed not to
  //! match any valid cpu number.
  static const unsigned int INVALID_CPU_NUMBER = (unsigned int)-1;

  explicit ITarget(const TraceFlags *traceFlags EMBDEBUG_ATTR_UNUSED){};
  virtual ~ITarget(){};

  virtual ResumeRes terminate() = 0;

  //! \brief Reset the cpu into a known state
  virtual ResumeRes reset(ResetType type) = 0;

  virtual uint64_t getCycleCount() const = 0;
  virtual uint64_t getInstrCount() const = 0;

  //! \brief Get the size of registers in the CPU in bytes.
  virtual int getRegisterSize() const = 0;

  //! \brief Read contents of a target register.
  //!
  //! \param[in]  reg   The register to read
  //! \param[out] value The read value, zero extended to the size of
  //!                   uint_reg_t.
  //! \return The size of the read register in bytes.
  virtual std::size_t readRegister(const int reg, uint_reg_t &value) = 0;

  // Write data to a target register.

  //! \brief Write the contents of a target register
  //!
  //! \param[in] reg   The register to write
  //! \param[in] value The value to write to the register, zero extended
  //!                  to the size of uint_reg_t.
  //! \return The size of the written register in bytes.
  virtual std::size_t writeRegister(const int reg, const uint_reg_t value) = 0;

  //! \brief Read data from the target's memory
  //!
  //! \param[in]  addr   The target memory address to be read from
  //! \param[out] buffer Buffer that the read memory will be written to. Must
  //!                    be non-null and large enough to hold the requested
  //!                    number of bytes.
  //! \param[in]  size   Number of bytes of memory to be read.
  //! \return The number of bytes read.
  virtual std::size_t read(const uint32_t addr, uint8_t *buffer,
                           const std::size_t size) = 0;

  //! \brief Write data to the target's memory
  //!
  //! \param[in] addr   The target memory address to be written to
  //! \param[in] buffer Buffer that the memory to write will be read from.
  //!                   Must be non-null.
  //! \param[in] size   Number of bytes of memory to be written to target
  //!                   memory from the buffer.
  //! \return The number of bytes written.
  virtual std::size_t write(const uint32_t addr, const uint8_t *buffer,
                            const std::size_t size) = 0;

  // Insert and remove a matchpoint (breakpoint or watchpoint) at the given
  // address.  Return value indicates whether the operation was successful.

  //! \brief Insert a matchpoint (breakpoint/watchpoint) at a given address
  //!
  //! \param[in] addr      Target memory address that the matchpoint will be
  //!                      inserted at.
  //! \param[in] matchType Type of the matchpoint (eg breakpoint/watchpoint)
  //! \return True if the matchpoint was successfully inserted.
  virtual bool insertMatchpoint(const uint32_t addr,
                                const MatchType matchType) = 0;

  //! \brief Remove a matchpoint (breakpoint/watchpoint) at a given address
  //!
  //! \param[in] addr      Target memory address of a matchpoint to be removed.
  //! \param[in] matchType Type of the matchpoint (eg breakpoint/watchpoint)
  //! \return True if the matchpoint was successfully removed.
  virtual bool removeMatchpoint(const uint32_t addr,
                                const MatchType matchType) = 0;

  //! \brief Pass through of an RSP command to the target
  //!
  //! This may be used for non-standard commands, or for getting extra
  //! information from the target.
  //!
  //! \param[in]  cmd    Command to be handled by the target. This string
  //!                    may not be empty.
  //! \param[out] stream Stream to hold the payload of the response packet.
  //! \return True if the command was handled; in this case \p stream contains
  //!         the response. False if the command was not handled.
  virtual bool command(const std::string cmd, std::ostream &stream) = 0;

  //! \brief Get a timestamp from the target
  //!
  //! This is primarily for cycle accurate models which may have a real
  //! concept of time, instead of just cycles and instructions.
  //!
  //! \return CPU time in seconds
  virtual double timeStamp() = 0;

  //! \brief The number of CPUs controlled by the target
  //!
  //! \return The number of CPUs. Must be >=1
  virtual unsigned int getCpuCount(void) = 0;

  // Which cpu is the "current" cpu?  This controls where reads and writes
  // will go.  Will always return a value greater than or equal to zero,
  // and less than the value returned by getCpuCount.

  //! \brief Get the currently selected CPU
  //!
  //! This controls where register and memory reads and writes will be directed.
  //!
  //! \return The index of the current CPU. This will always be >=0 and
  //!         \< getCpuCount()
  virtual unsigned int getCurrentCpu(void) = 0;

  // Set the "current" cpu.  This controls where reads and writes will go,
  // which registers will be accessed, etc.  The value passed must be
  // greater than or equal to zero, and less than the value returned by
  // getCpuCount.  Behaviour for an invalid value is undefined.

  //! \brief Set the current CPU
  //!
  //! This controls where register and memory reads and writes will be directed.
  //!
  //! \param[in] index The index of the current CPU. This must be >=0 and
  //!                  \< getCpuCount()
  virtual void setCurrentCpu(unsigned int index) = 0;

  //! \brief Prepare each core to be resumed
  //!
  //! A ResumeType is provided for each of the cores to determine what that
  //! core should do when resume() is called.
  //!
  //! \param[in] actions A vector of actions to be performed, equal in
  //!                    length to getCpuCount()
  //! \return True if the cores were successfully prepared.
  virtual bool prepare(const std::vector<ResumeType> &actions) = 0;

  //! \brief Move cores that are going to do something into a running state
  //!
  //! \return True if the cores were successfully resumed.
  virtual bool resume(void) = 0;

  //! \brief Wait for some stop event to occur on a resumed core.
  //!
  //! Once one core stops all remaining cores should also be halted, and
  //! the state of each of the cores returned in \p results.
  //!
  //! \param[out] results A vector holding the state of all of the cores. This
  //!                     must be cleared and repopulated by the target and
  //!                     must contain one entry for each of the cores.
  virtual WaitRes wait(std::vector<ResumeRes> &results) = 0;

  //! \brief Halt all running cores
  //!
  //! \return True if all cores were successfully halted, false otherwise.
  //!         Failure to halt will generally be a fatal error.
  virtual bool halt(void) = 0;

private:
  // Don't allow the default constructors

  ITarget(){};
  ITarget(const ITarget &){};
};

std::ostream &operator<<(std::ostream &s, ITarget::ResumeType p);
std::ostream &operator<<(std::ostream &s, ITarget::ResumeRes p);
std::ostream &operator<<(std::ostream &s, ITarget::MatchType p);

} // namespace EmbDebug

#endif
