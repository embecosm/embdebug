// GDB Timeout: definition
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include "embdebug/Timeout.h"

using std::chrono::duration;

using namespace EmbDebug;

//! Constructor for no timeout.

Timeout::Timeout()
    : mTimeoutType(Timeout::Type::NONE), mRealTimeout(duration<double>::zero()),
      mCycleTimeout(0) {} // Timeout::Timeout ()

//! Constructor for a wall clock GDB server timeout.

//! @param[in] realTimeout  Wallclock timeout to use.

Timeout::Timeout(const duration<double> realTimeout)
    : mTimeoutType(Timeout::Type::REAL), mRealTimeout(realTimeout),
      mCycleTimeout(0) {} // Timeout::Timeout ()

//! Constructor for a cycle count GDB server timeout.

//! @param[in] cycleTimeout  Cycle count timeout to use.

Timeout::Timeout(const uint64_t cycleTimeout)
    : mTimeoutType(Timeout::Type::CYCLE),
      mRealTimeout(duration<double>::zero()), mCycleTimeout(cycleTimeout) {
} // Timeout::Timeout ()

//! Destructor.

//! Empty at present

Timeout::~Timeout() {} // Timeout::~Timeout ()

//! Accessor: Set no timeout

void Timeout::clearTimeout() {
  mTimeoutType = Type::NONE;
  mRealTimeout = duration<double>::zero();
  mCycleTimeout = 0;

} // Timeout::clearTimeout ()

//! Accessor: Get wall clock timeout.

//! @return The wall clock timeout, which will be zero if we are not using
//!         wall clock timeouts.

std::chrono::duration<double> Timeout::realTimeout() const {
  return mRealTimeout;

} // Timeout::realTimeout ()

//! Accessor: Set wall clock timeout.

//! @param[in] realTimeout The wall clock timeout to set

void Timeout::realTimeout(const duration<double> realTimeout) {
  mTimeoutType = Type::REAL;
  mRealTimeout = realTimeout;
  mCycleTimeout = 0;

} // Timeout::realTimeout ()

//! Accessor: Get cycle count timeout.

//! @return The cycle count timeout, which will be zero if we are not using
//!         wall clock timeouts.

uint64_t Timeout::cycleTimeout() const {
  return mCycleTimeout;

} // Timeout::cycleTimeout ()

//! Accessor: Set cycle count timeout.

//! @param[in] realTimeout The cycle count timeout to set

void Timeout::cycleTimeout(const uint64_t cycleTimeout) {
  mTimeoutType = Type::CYCLE;
  mRealTimeout = duration<double>::zero();
  mCycleTimeout = cycleTimeout;

} // Timeout::cycleTimeout ()

//! Do we have a timeout set?

//! We use this mostly to determine if no timeout has been set - i.e. we
//! should run for ever.

//! @return  TRUE if eithe

bool Timeout::haveTimeout() const {
  return mTimeoutType != Type::NONE;

} // Timeout::isCycleTimeout ()

//! Is this a wall clock timeout?

//! @return  TRUE if this is a wall clock timeout.

bool Timeout::isRealTimeout() const {
  return mTimeoutType == Type::REAL;

} // Timeout::isRealTimeout ()

//! Is this a cycle count timeout?

//! @return  TRUE if this is a cycle count timeout.

bool Timeout::isCycleTimeout() const {
  return mTimeoutType == Type::CYCLE;

} // Timeout::isCycleTimeout ()

//! Set a timestamp now for the current CPU

//! @param[in] cpu  The CPU to which the timestamp relates

void Timeout::timeStamp(ITarget *cpu) {
  mRealStamp = std::chrono::system_clock::now();
  mCycleStamp = cpu->getCycleCount();

} // Timeout::timeStamp ()

//! Are we more than duration past the time stamp

//! We base this on the type of timeout we are.

//! @param[in] cpu  The CPU to which the timestamp relates
//! @return  TRUE if we have timed out, FALSE otherwise.

bool Timeout::timedOut(ITarget *cpu) const {
  switch (mTimeoutType) {
  case Type::NONE:
    return false;

  case Type::REAL:
    return std::chrono::system_clock::now() > (mRealStamp + mRealTimeout);

  case Type::CYCLE:
    return cpu->getCycleCount() > (mCycleStamp + mCycleTimeout);

  default:

    std::cerr << "*** ABORT: Impossible clock type in timedOut" << std::endl;
    abort();
  }
} // Timeout::timedOut ()
