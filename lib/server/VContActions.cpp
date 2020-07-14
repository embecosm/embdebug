// Class to handle decoding vCont packets.
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2017-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include "VContActions.h"
#include "Utils.h"

#include <cassert>
#include <cstring>
#include <string>
#include <vector>

using std::cerr;
using std::endl;
using std::string;
using std::vector;

using namespace EmbDebug;

// Parse vCont packet in STR, setup the state of this object.  Return true
// if everything parsed correctly, otherwise return false.  If we return
// false then the state of this object is undefined.

bool VContActions::parse(const char *str) {
  vector<string> tokens;

  // Skip the leading 'vCont;' header.
  str += strlen("vCont;");
  Utils::split(str, ";", tokens);

  for (auto it = tokens.begin(); it != tokens.end(); ++it) {
    const char *tmp;
    std::size_t pos;
    Ptid ptid(Ptid::PTID_ALL, Ptid::PTID_ALL);

    // Find the ':', the start of the pid/tid descriptor.
    pos = it->find(':');
    if (pos != std::string::npos) {
      // Convert the pid/tid into a decoded object.
      tmp = it->c_str() + pos + 1;
      if (!ptid.decode(tmp))
        return false;

      if (ptid.pid() == 0) {
        cerr << "Warning: found pid == 0 in vCont '" << str << "'" << endl;
        return false;
      }
    }

    // Store the details into the actions vector.
    std::string action = *it;
    mActions.push_back(std::make_pair(action, ptid));
  }

  return true;
}

// Return true if this vCont packet effects more than one core.  This is
// hopefully a temporary measure.  At some point we will want to support
// running many cores at once, in which case we'll no longer care how many
// cores a vCont packet refers to.

bool VContActions::effectsMultipleCores(void) const {
  unsigned int num = 0;

  assert(valid());

  for (auto it = mActions.begin(); it != mActions.end(); ++it) {
    unsigned int pid = it->second.pid();

    assert(pid != 0);
    // The '-1' pid counts as all cores.  We call this "many" even though
    // we may only have one core alive at this point.
    if (pid == ((unsigned int)-1))
      return true;
    // No cached core yet, so cache the one from this action.
    else if (num == 0)
      num = pid;
    // If we don't match the cached core, then we are asking many cores
    // to perform an action.
    else if (pid != num)
      return true;
  }

  return false;
}

// Return the action letter for core NUM.  This ignores any signals that
// might also have been sent in the vCont as (for now) we do nothing with
// these anyway.  This might change in the future.

char VContActions::getCoreAction(unsigned int num) const {
  for (auto it = mActions.begin(); it != mActions.end(); ++it) {
    unsigned int pid = it->second.pid();

    assert(pid != 0);
    if ((pid == ((unsigned int)-1)) || pid == num)
      return (it->first.c_str())[0];
  }

  return '\0';
}
