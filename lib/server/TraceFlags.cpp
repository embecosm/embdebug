// GDB RSP server: implementation
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include <iostream>
#include <sstream>

#include "TraceFlags.h"

using std::cerr;
using std::endl;
using std::ostringstream;
using std::string;

using namespace EmbDebug;

//! Allocate the static map of flags.

std::map<const string, TraceFlags::FlagInfo> TraceFlags::sFlagInfo;

//! Constructor for the trace flags.

TraceFlags::TraceFlags() {
  // Initialize the map of flag info

  sFlagInfo["rsp"] = {false, nullptr, 0};
  sFlagInfo["conn"] = {false, nullptr, 0};
  sFlagInfo["break"] = {false, nullptr, 0};
  sFlagInfo["vcd"] = {false, nullptr, 0};
  sFlagInfo["silent"] = {false, nullptr, 0};
  sFlagInfo["disas"] = {false, nullptr, 0};
  sFlagInfo["qdisas"] = {false, nullptr, 0};
  sFlagInfo["dflush"] = {false, nullptr, 0};
  sFlagInfo["mem"] = {false, nullptr, 0};
  sFlagInfo["exec"] = {false, nullptr, 0};
  sFlagInfo["verbosity"] = {false, nullptr, 0};
  sFlagInfo["ipg"] = {false, nullptr, 50};
}

//! Destructor for the trace flags.

TraceFlags::~TraceFlags() {
  for (auto it = sFlagInfo.begin(); it != sFlagInfo.end(); it++)
    if (nullptr != it->second.val) {
      delete it->second.val;
      it->second.val = nullptr;
    }

  sFlagInfo.clear();
}

//! Is RSP tracing enabled?

//! @return  TRUE if the RSP tracing flag is set, FALSE otherwise

bool TraceFlags::traceRsp() const { return flagState("rsp"); }

//! Is connection tracing enabled?

//! @return  TRUE if the CONN tracing flag is set, FALSE otherwise

bool TraceFlags::traceConn() const { return flagState("conn"); }

//! Is breakpoint tracing enabled?

//! @return  TRUE if the BREAK tracing flag is set, FALSE otherwise

bool TraceFlags::traceBreak() const { return flagState("break"); }

//! Is VCD tracing enabled?

//! @return  TRUE if the VCD tracing flag is set, FALSE otherwise

bool TraceFlags::traceVcd() const { return flagState("vcd"); }

//! Is silent running enabled?

//! @return  TRUE if the SILENT tracing flag is set, FALSE otherwise

bool TraceFlags::traceSilent() const { return flagState("silent"); }

//! Is disassembly enabled?

//! @return  TRUE if the DISAS tracing flag is set, FALSE otherwise

bool TraceFlags::traceDisas() const { return flagState("disas"); }

//! Is quiet disassembly enabled?

//! @return  TRUE if the QDISAS tracing flag is set, FALSE otherwise

bool TraceFlags::traceQdisas() const { return flagState("qdisas"); }

//! Is per step disassembly flushing enabled?

//! @return  TRUE if the DFLUSH tracing flag is set, FALSE otherwise

bool TraceFlags::traceDflush() const { return flagState("dflush"); }

//! Is per step disassembly flushing enabled?

//! @return  TRUE if the MEM tracing flag is set, FALSE otherwise

bool TraceFlags::traceMem() const { return flagState("mem"); }

//! Is execution tracing enabled ?

//! @return  TRUE if the EXEC tracing flag is set, FALSE otherwise

bool TraceFlags::traceExec() const { return flagState("exec"); }

int32_t TraceFlags::traceVerbosity() const {
  return flagNumericVal("verbosity");
}

int32_t TraceFlags::traceIPG() const { return flagNumericVal("ipg"); }

//! Is this a real flag

//! @param[in] flagName  Case insensitive name to check.
//! @return  TRUE if this is a valid flag name, FALSE otherwise.

bool TraceFlags::isFlag(const string &flagName) const {
  return sFlagInfo.find(flagName) != sFlagInfo.end();
}

bool TraceFlags::isNumericFlag(const string &flagName) const {
  return ((flagName == "verbosity") || (flagName == "ipg"));
}

//! Set a named flags state and value

//! We don't allow setting of flags that do not already exist.

//! @note We don't allow a default for flagVal to avoid the risk of
//!       accidentally using this function instead of flagState.

//! @param[in] flagName   The name of the flag (case insensitive)
//! @param[in] flagState  Flag state to set
//! @param[in] flagVal    Associated value

void TraceFlags::flag(const string &flagName, const bool flagState,
                      const string &flagVal, const bool numeric) {
  if (isFlag(flagName)) {
    sFlagInfo[flagName].state = flagState;

    if (nullptr != sFlagInfo[flagName].val)
      delete (sFlagInfo[flagName].val);

    int32_t numeric_val = 0;

    if (numeric) {
      try {
        numeric_val = std::stoi(flagVal);
      } catch (const std::logic_error &) {
        cerr << "*** ERROR *** Failed to parse numeric value of " << flagVal
             << endl;
        exit(EXIT_FAILURE);
      }
    }

    sFlagInfo[flagName].val = new string(flagVal);
    sFlagInfo[flagName].numeric_val = numeric_val;
  } else {
    cerr << "*** ERROR *** Attempt to set bad trace flag" << endl;
    exit(EXIT_FAILURE);
  }
}

//! Set a named flag's state

//! This leaves the flag's value unchanged

//! The flag is assumed to exist, and if it does not bad things will happen!

//! @param[in] flagName   The name of the flag (case insensitive)
//! @param[in] flagState  Flag state to set

void TraceFlags::flagState(const string &flagName, const bool flagState) {
  if (isFlag(flagName)) {
    sFlagInfo[flagName].state = flagState;
  } else {
    cerr << "*** ERROR *** Attempt to set state of bad trace flag" << endl;
    exit(EXIT_FAILURE);
  }
}

//! Get the state of a named flag

//! The flag is assumed to exist, and if it does not bad things will happen!

//! @param[in] flagName  The name of the flag (case insensitive)
//! return  State of the flag.

bool TraceFlags::flagState(const string &flagName) const {
  if (isFlag(flagName))
    return sFlagInfo[flagName].state;
  else {
    cerr << "*** ERROR *** Attempt to get state of bad trace flag" << endl;
    exit(EXIT_FAILURE);
  }
}

//! Set a named flag's value

//! This leaves the flag's state unchanged

//! The flag is assumed to exist, and if it does not bad things will happen!

//! @param[in] flagName   The name of the flag (case insensitive)
//! @param[in] flagVal  Flag value to set

void TraceFlags::flagVal(const string &flagName, const string &flagVal) {
  if (isFlag(flagName)) {
    if (nullptr != sFlagInfo[flagName].val)
      delete (sFlagInfo[flagName].val);

    sFlagInfo[flagName].val = new string(flagVal);
  } else {
    cerr << "*** ERROR *** Attempt to set value of bad trace flag" << endl;
    exit(EXIT_FAILURE);
  }
}

//! Get the value of a named flag

//! The flag is assumed to exist, and if it does not bad things will happen!

//! @param[in] flagName  The name of the flag (case insensitive)
//! return  Value of the flag.

string TraceFlags::flagVal(const string &flagName) const {
  if (isFlag(flagName)) {
    if (sFlagInfo[flagName].val != nullptr)
      return *(sFlagInfo[flagName].val);
    else
      return string();
  } else {
    cerr << "*** ERROR *** Attempt to get value of bad trace flag" << endl;
    exit(EXIT_FAILURE);
  }
}

//! Get the numeric value of a named flag

//! The flag is assumed to exist, and if it does not bad things will happen!

//! @param[in] flagName  The name of the flag (case insensitive)
//! return  Value of the flag.

int32_t TraceFlags::flagNumericVal(const string &flagName) const {
  if (isFlag(flagName)) {
    return sFlagInfo[flagName].numeric_val;
  } else {
    cerr << "*** ABORT *** Attempt to get value of bad trace flag" << endl;
    exit(EXIT_FAILURE);
  }
}

//! Parse a command line argument

//! This is the argument to the -t option, which either takes one of the two
//! forms:

//!   - <text>

//!     <text> is the name of a flag to set

//!   - <text1>=<text2>

//!     <text1> is the name of a flag to set and <text2> the value to
//!     associate with it.

//! @param[in] arg  Argument to parse
//! @return TRUE if successfully parsed, FALSE otherwise.

bool TraceFlags::parseArg(string &arg) {
  size_t idx = arg.find('=');

  if (idx == string::npos)
    if (isFlag(arg)) {
      flag(arg, true, "", false);
      return true;
    } else
      return false;
  else {
    // Split the strings

    string key = arg.substr(0, idx);
    string value = arg.substr(idx + 1);

    if (isFlag(key)) {
      flag(key, true, value, isNumericFlag(key));
      return true;
    } else
      return false;
  }
}

//! Dump out all the trace state

//! Useful for debugging.

//! @return A string with all the trace flags and their state, one per line.

string TraceFlags::dump() {
  ostringstream oss;

  for (auto it = sFlagInfo.begin(); it != sFlagInfo.end(); it++) {
    oss << it->first << ": " << ((it->second.state) ? "ON" : "OFF");

    if (nullptr != it->second.val)
      oss << " (associated val = \"" << *(it->second.val) << "\" / "
          << (it->second.numeric_val) << ")";

    oss << endl;
  }

  return oss.str();
}
