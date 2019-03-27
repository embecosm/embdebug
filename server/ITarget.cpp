// Generic GDB RSP server interface: definition.
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2008-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Even though ITarget is an abstract class, it requires implementation of the
// stream operators to allow its public scoped enumerations to be output.
// ----------------------------------------------------------------------------

#include "embdebug/ITarget.h"

using namespace EmbDebug;

namespace EmbDebug {

//! Output operator for ResumeType enumeration

//! @param[in] s  The stream to output to.
//! @param[in] p  The ResumeType value to output.
//! @return  The stream with the item appended.

std::ostream &operator<<(std::ostream &s, ITarget::ResumeType p) {
  const char *name;

  switch (p) {
  case ITarget::ResumeType::STEP:
    name = "step";
    break;
  case ITarget::ResumeType::CONTINUE:
    name = "continue";
    break;
  case ITarget::ResumeType::NONE:
    name = "none";
    break;
  default:
    name = "unknown";
    break;
  }

  return s << name;
}

//! Output operator for ResumeRes enumeration

//! @param[in] s  The stream to output to.
//! @param[in] p  The ResumeRes value to output.
//! @return  The stream with the item appended.

std::ostream &operator<<(std::ostream &s, ITarget::ResumeRes p) {
  const char *name;

  switch (p) {
  case ITarget::ResumeRes::NONE:
    name = "none";
    break;
  case ITarget::ResumeRes::SUCCESS:
    name = "success";
    break;
  case ITarget::ResumeRes::FAILURE:
    name = "failure";
    break;
  case ITarget::ResumeRes::INTERRUPTED:
    name = "interrupted";
    break;
  case ITarget::ResumeRes::TIMEOUT:
    name = "timeout";
    break;
  case ITarget::ResumeRes::SYSCALL:
    name = "syscall";
    break;
  case ITarget::ResumeRes::STEPPED:
    name = "stepped";
    break;
  default:
    name = "unknown";
    break;
  }

  return s << name;
}

} // namespace EmbDebug

//! Output operator for MatchType enumeration

//! @param[in] s  The stream to output to.
//! @param[in] p  The MatchType value to output.
//! @return  The stream with the item appended.

std::ostream &operator<<(std::ostream &s, ITarget::MatchType p) {
  const char *name;

  switch (p) {
  case ITarget::MatchType::BREAK:
    name = "breakpoint";
    break;
  case ITarget::MatchType::BREAK_HW:
    name = "hardware breakpoint";
    break;
  case ITarget::MatchType::WATCH_WRITE:
    name = "write watchpoint";
    break;
  case ITarget::MatchType::WATCH_READ:
    name = "read watchpoint";
    break;
  case ITarget::MatchType::WATCH_ACCESS:
    name = "access watchpoint";
    break;
  default:
    name = "unknown";
    break;
  }

  return s << name;
}
