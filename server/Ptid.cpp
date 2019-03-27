// Process/thread ID implementation: definition
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include <cstring>

#include "Ptid.h"
#include "embdebug/Utils.h"

using std::cerr;
using std::endl;

using namespace EmbDebug;

//! Constructor for the PTID.

//! @param[in]  pid  The process ID
//! @param[in]  tid  The tread ID

Ptid::Ptid(const int pid, const int tid) : mPid(pid), mTid(tid) {
  if (!validate())
    cerr << "Invalid PTID created: " << *this << endl;
}

//! Destructor.

//! For now nothing to do

Ptid::~Ptid() {}

//! Accessor to set the PID.

//! @param[in] The PID value to use

void Ptid::pid(const int _pid) { mPid = _pid; }

//! Accessor to get the PID.

//! @results  The value of the PID

int Ptid::pid() const { return mPid; }

//! Accessor to set the TID.

//! @param[in] The TID value to use

void Ptid::tid(const int _tid) { mTid = _tid; }

//! Accessor to get the TID.

//! @results  The value of the TID

int Ptid::tid() const { return mTid; }

//! Select a specific PTID

//! In some cases the client tells us to use any PTID, and we need to
//! crystalize this to a specific PTID.

//! In general, specifying all threads should not be used here, so we return
//! this as an error.

//! @param[in] defaultPid  The value of PID to use if ANY is specified.
//! @param[in] defaultTid  The value of TID to use if ANY is specified.
//! @return TRUE if we did crystalize, FALSE otherwise. If we return FALSE,
//!         the values of mPid and mTid are not changed.

bool Ptid::crystalize(int defaultPid, int defaultTid) {
  int pid;
  int tid;

  if (!validate()) // CHeck we don't have nonsense
  {
    cerr << "Warning: Attempt to crystalize invalid PTID: " << *this << endl;
    return false;
  }

  switch (mPid) {
  case PTID_INV:
  case PTID_ALL:

    cerr << "Warning: Can't crystalize PID: " << *this << endl;
    return false;

  case PTID_ANY:

    pid = defaultPid;
    break;

  default:

    pid = mPid;
    break;
  }

  switch (mTid) {
  case PTID_INV:
  case PTID_ALL:

    cerr << "Warning: Can't crystalize TID: " << *this << endl;
    return false;

  case PTID_ANY:

    tid = defaultTid;
    break;

  default:

    tid = mTid;
    break;
  }

  // All OK, set up crystalized PTID.

  mPid = pid;
  mTid = tid;

  return true;
}

//! Is this a valid PTID

//! This should never fail - it is a sanity check for corruption. @note that
//! PTID_INV, which is an extension is valid in this context.

//! @return  TRUE if this is a valid PTID, FALSE otherwise.

bool Ptid::validate() {
  if (!((mPid > 0) || (PTID_ANY == mPid) || (PTID_ALL == mPid) ||
        (PTID_INV == mPid)))
    return false;
  else if (!((mTid > 0) || (PTID_ANY == mTid) || (PTID_ALL == mTid) ||
             (PTID_INV == mTid)))
    return false;
  else
    return true;
}

//! Break out a PTID

//! Syntax is:

//!     <tid>
//!     p<pid>
//!     p<pid>.<tid>

//! Permitted values of <pid> and <tid> are:

//! If the PID is not specified, then it is left unchanged. If the TID is not
//! specified, it is set to ALL.

//!    literal "0" to mean ANY thread/process
//!    literal "-1" to mean ALL threads/processes
//!    a hex encoded positive number.

//! The result is placed into mPid and mTid

//! @param[in] buf  The string to parse
//! @return TRUE if successfully parsed, FALSE otherwise (in which case the
//!         values in mPid and mTid are unchanged).

bool Ptid::decode(const char *buf) {
  int pid;
  int tid;
  const char *ptr; // Pointer into the buffer

  // break out formats

  if (buf[0] != 'p') {
    // Simplest format. Just a TID. We leave PID unchanged.

    pid = mPid;
    tid = decodeField(buf, strlen(buf));

    if (PTID_INV == tid) {
      cerr << "Warning: Invalid TID, " << buf << ": ignored." << endl;
      return false;
    }
  } else if (nullptr == (ptr = strchr(buf, '.'))) {
    // Just a PID

    pid = decodeField(&(buf[1]), strlen(&(buf[1])));
    tid = PTID_ALL;

    if (PTID_INV == pid) {
      cerr << "Warning: Invalid PID, " << buf << ": ignored." << endl;
      return false;
    }
  } else {
    // A PTID

    pid = decodeField(&(buf[1]), ptr - buf - 1);
    tid = decodeField(&(ptr[1]), strlen(&(ptr[1])));

    if ((PTID_INV == pid) || (PTID_INV == tid)) {
      cerr << "Warning: Invalid PTID, " << buf << ": ignored." << endl;
      return false;
    }
  }

  // Rule out an invalid combination.

  if ((PTID_ALL == pid) & ((PTID_ALL == tid) || (PTID_ANY == tid))) {
    cerr << "Warning: Invalid PTID, " << buf << ": ignored." << endl;
    return false;
  }

  mPid = pid;
  mTid = tid;
  return true;
}

//! Break out a single PTID field

//! Permitted values are

//!    literal "0" to mean ANY thread/process
//!    literal "-1" to mean ALL threads/processes
//!    a hex encoded positive number.

//! The result is returned, with the additional value PTID_INV used to
//! indicate a failure in parsing.

//! @param[in] buf  The string to parse
//! @param[in] len  Length of the string to parse
//! @return The field value

int Ptid::decodeField(const char *buf, const std::size_t len) const {
  if ((1 == len) && ('0' == buf[0]))
    return PTID_ANY;
  else if ((2 == len) && ('-' == buf[0]) && ('1' == buf[1]))
    return PTID_ALL;
  else if (Utils::isHexStr(buf, len))
    return (int)(Utils::hex2Val(buf, len));
  else
    return PTID_INV;
}

//! Encode a PTID

//! Convenience overload to use the current PTID

//! @param[out] buf  Buffer for the encoding (assumed large enough)
//! @return TRUE if we succeeded, FALSE otherwise.

bool Ptid::encode(char *buf) {
  buf[0] = 'p';

  int off = 1;
  std::size_t len = encodeField(&(buf[off]), mPid);

  if (0 == len)
    return false;

  off += len;
  buf[off] = '.';
  off++;

  len = encodeField(&(buf[off]), mTid);
  buf[off + len] = '\0';

  return (0 != len);
}

//! Encode a PTID field

//! Permitted Valid values are PTID_ALL (-1), PTID_ANY (0) or any positive
//! number.

//! We assume that PTID values are never greater than 2^31 - 1.

//! @param[out] buf  Buffer for the encoding (assumed large enough)
//! @param[in]  ptid  Process ID
//! @return length of the string if we succeed, 0 otherwise.

std::size_t Ptid::encodeField(char *buf, int ptid) {
  if (ptid < PTID_ALL)
    return 0;

  switch (ptid) {
  case PTID_ALL:

    buf[0] = '-';
    buf[1] = '1';
    return 2;

  case PTID_ANY:

    buf[0] = '0';
    return 1;

  default:

    return Utils::val2Hex(ptid, buf);
  }
}

//! Output operator for Ptid class

//! @param[in] s  The stream to output to.
//! @param[in] p  The Ptid value to output.
//! @return  The stream with the item appended.

namespace EmbDebug {

std::ostream &operator<<(std::ostream &s, Ptid p) {
  return s << "{" << p.mPid << "," << p.mTid << "}";
}

} // namespace EmbDebug
