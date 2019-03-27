// Process/thread ID implementation: declaration
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

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
};

} // namespace EmbDebug

#endif
