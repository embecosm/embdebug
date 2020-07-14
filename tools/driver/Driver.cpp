// GDBServer driver
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4018 4267) // Workaround cxxopts.hpp issues #147, #148
#endif
#include "cxxopts-2.1.2/include/cxxopts.hpp"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "Init.h"
#include "TraceFlags.h"
#include "embdebug/ITarget.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

using std::cerr;
using std::cout;
using std::endl;
using std::ostream;
using std::string;

using namespace EmbDebug;

static const std::string gdbserver_name = "embdebug";

// FIXME: Extract the version from CMake
string get_version() {
  std::stringstream s;
  s << gdbserver_name << " version "
    << "PACKAGE_VERSION"
    << " of "
    << "PACKAGE_NAME"
    << " ("
    << "GIT_VERSION"
    << ")";
  return s.str();
}

string trace_help() {
  return "\nThe -t/--trace option may appear multiple times. Trace flags "
         "are:\n\n"
         "  rsp               Trace RSP packets\n"
         "  conn              Trace RSP connection handling\n"
         "  break             Trace breakpoint handling\n"
         "  vcd               Generate a Value Change Dump\n"
         "  silent            Minimize informative messages (synonym for -q)\n"
         "  disas=<filename>  Disassemble each instruction executed\n"
         "  qdisas            Make 'disas' quieter, only trace instructions\n"
         "  dflush            Flush disassembly to file after each step\n"
         "  mem               Trace multicore memory access\n"
         "  exec              Trace core execution and halting\n"
         "  verbosity=<n>     Trace verbosity level\n";
}

typedef ITarget *(*create_target_func)(TraceFlags *);

#ifndef _WIN32
ITarget *load_target_so(string soname, TraceFlags *traceFlags) {
  void *handle = dlopen(soname.c_str(), RTLD_NOW);
  if (!handle) {
    cerr << "Failed to load " << soname << ": " << dlerror() << endl;
    exit(EXIT_FAILURE);
  }
  // Before attempting to create a target, verify that the library is for this
  // ITarget version
  uint64_t (*api_version)() = (uint64_t(*)())dlsym(handle, "ITargetVersion");
  if (!api_version) {
    cerr << "Failed to look up ITargetVersion: " << dlerror() << endl;
    exit(EXIT_FAILURE);
  }
  if (api_version() != ITarget::CURRENT_API_VERSION) {
    cerr << "Incompatible ITarget versions: Target declared version "
         << api_version() << ", expected " << ITarget::CURRENT_API_VERSION
         << endl;
    exit(EXIT_FAILURE);
  }
  create_target_func create_target =
      (create_target_func)dlsym(handle, "create_target");
  if (!create_target) {
    cerr << "Failed to look up create_target function: " << dlerror() << endl;
    exit(EXIT_FAILURE);
  }
  return create_target(traceFlags);
}
#endif

#ifdef _WIN32
ITarget *load_target_dll(string dllname, TraceFlags *traceFlags) {
  HMODULE handle = LoadLibrary(dllname.c_str());
  if (!handle) {
    cerr << "Failed to load " << dllname << "." << endl;
    exit(EXIT_FAILURE);
  }
  // Before attempting to create a target, verify that the library is for this
  // ITarget version
  uint64_t (*api_version)() =
      (uint64_t(*)())GetProcAddress(handle, "ITargetVersion");
  if (!api_version) {
    cerr << "Failed to look up ITargetVersion." << endl;
    exit(EXIT_FAILURE);
  }
  if (api_version() != ITarget::CURRENT_API_VERSION) {
    cerr << "Incompatible ITarget versions: Target declared version "
         << api_version() << ", expected " << ITarget::CURRENT_API_VERSION
         << endl;
    exit(EXIT_FAILURE);
  }
  create_target_func create_target =
      (create_target_func)GetProcAddress(handle, "create_target");
  if (!create_target) {
    cerr << "Failed to look up create_target function." << endl;
    exit(EXIT_FAILURE);
  }
  return create_target(traceFlags);
}
#endif

int main(int argc, char *argv[]) {
  string soName;
  bool from_stdin;
  TraceFlags traceFlags;
  bool withLockstep;
  int rspPort = 0;

  cxxopts::Options options("embdebug", "GDBServer");
  options.add_options()("q,silent",
                        "Don't emit informational messages on stdout");
  options.add_options()("h,help", "Display help message");
  options.add_options()("t,trace", "Trace item",
                        cxxopts::value<std::vector<std::string>>(), "<flag>");
  options.add_options()(
      "s,stdin", "Communicate with GDB via pipe instead of TCP socket",
      cxxopts::value<bool>(from_stdin)->default_value("false"));
  options.add_options()("v,version", "Show version information");
  options.add_options()(
      "l,lockstep", "Enable lockstep debugging",
      cxxopts::value<bool>(withLockstep)->default_value("false"));
  options.add_options()("soname", "Shared object containing model",
                        cxxopts::value<string>(soName), "<shared object>");
  options.add_options()("rsp-port", "Port to listen on",
                        cxxopts::value<string>(), "<num>");

  options.positional_help("[rsp-port]");
  options.parse_positional({"rsp-port"});

  try {
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      cerr << options.help() << trace_help() << endl;
      return EXIT_SUCCESS;
    }

    if (result.count("silent"))
      traceFlags.flagState("silent", true);

    if (result.count("version")) {
      cerr << get_version() << endl;
      return EXIT_SUCCESS;
    }

    if (!result.count("soname")) {
      cerr << "No soname specified, cannot create target" << endl;
      return EXIT_FAILURE;
    }

    if (result.count("rsp-port")) {
      string token = result["rsp-port"].as<std::string>();
      // In GDB when connecting to a local gdbserver over a socket the
      // syntax is 'target remote :PORT'.  Sometimes users then try to
      // start gdbserver with 'embdebug -c CORE :PORT'.  Given
      // such a common mistake, lets try to help the user out here.
      if (token[0] == ':')
        token = token.substr(1, string::npos);

      try {
        rspPort = std::stoi(token);
      } catch (std::logic_error &) {
        cerr << "ERROR: failed to parse port number from: " << token << endl;
        return EXIT_FAILURE;
      }

      if (rspPort < 0 || rspPort > UINT16_MAX) {
        cerr << "ERROR: invalid port number: " << rspPort << endl;
        return EXIT_FAILURE;
      }
    } else {
      cerr << "NOTE: No port number found - using ephemeral port" << endl;
    }

    if (result.count("trace")) {
      for (auto flag : result["trace"].as<std::vector<std::string>>()) {
        if (!traceFlags.parseArg(flag)) {
          cerr << "ERROR: Bad trace flag " << flag << endl;
          cerr << options.help();
          return EXIT_FAILURE;
        }
      }
    }
  } catch (cxxopts::OptionException &e) {
    cerr << e.what() << endl;
    cerr << options.help();
    return EXIT_FAILURE;
  }

  ITarget *target;

  // If a user provides just the target name, build the correct soname from it.
#ifdef _WIN32
  if (soName.rfind(".dll") == std::string::npos)
    soName = "embdebug-target-" + soName + ".dll";
#elif __APPLE__
  if (soName.rfind(".dylib") == std::string::npos)
    soName = "libembdebug-target-" + soName + ".dylib";
#else
  if (soName.rfind(".so") == std::string::npos)
    soName = "libembdebug-target-" + soName + ".so";
#endif

  cerr << "Loading ITarget interface from dynamic library: " << soName << endl;
#ifdef _WIN32
  target = load_target_dll(soName, &traceFlags);
#else
  target = load_target_so(soName, &traceFlags);
#endif

  return init(target, &traceFlags, from_stdin, rspPort, false);
}
