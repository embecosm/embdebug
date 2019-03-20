// GDB RSP server: definition

// Copyright (C) 2009, 2013  Embecosm Limited <info@embecosm.com>

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

#ifndef TRACE_FLAGS_H
#define TRACE_FLAGS_H

#include <cstring>
#include <map>
#include <string>

namespace EmbDebug {

//! Class for trace flags

//! The interface only uses names of flags. Flags have a state (TRUE or
//! FALSE) and optionally a value (text).

class TraceFlags {
public:
  // Constructor and destructor

  TraceFlags();
  ~TraceFlags();

  // Accessors

  bool traceRsp() const;
  bool traceConn() const;
  bool traceBreak() const;
  bool traceVcd() const;
  bool traceSilent() const;
  bool traceDisas() const;
  bool traceQdisas() const;
  bool traceDflush() const;
  bool traceMem() const;
  bool traceExec() const;
  int32_t traceVerbosity() const;
  int32_t traceIPG() const;
  bool isFlag(const std::string &flagName) const;
  bool isNumericFlag(const std::string &flagName) const;
  void flag(const std::string &flagName, const bool flagState,
            const std::string &flagVal, const bool numeric);
  void flagState(const std::string &flagName, const bool flagState);
  bool flagState(const std::string &flagName) const;
  void flagVal(const std::string &flagName, const std::string &flagVal);
  std::string flagVal(const std::string &flagName) const;
  int32_t flagNumericVal(const std::string &flagName) const;

  // Helper to parse argument from command line

  bool parseArg(std::string &arg);

  // Debugging method

  std::string dump();

private:
  //! All the flag state

  struct FlagInfo {
    bool state;
    std::string *val;
    int32_t numeric_val;
  };

  //! All the info about flags. This is a map from the name of the flag.

  static std::map<const std::string, FlagInfo> sFlagInfo;

}; // TraceFlags ()

} // namespace EmbDebug

#endif // TRACE_FLAGS_H
