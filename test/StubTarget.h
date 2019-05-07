// Stub implementation of the ITarget interface
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef STUB_TARGET_H
#define STUB_TARGET_H

#include "embdebug/Compat.h"
#include "embdebug/ITarget.h"

namespace EmbDebug {

class TraceFlags;
}

using namespace EmbDebug;

// This is a stub implementation of the ITarget interface which can be
// specialized for different tests. Importantly it will throw an exception
// if a test uses any part of the interface which is not explicitly
// implemented.
class StubTarget : public ITarget {
public:
  StubTarget(const TraceFlags *traceFlags) : ITarget(traceFlags) {}
  ~StubTarget() override{};

  ResumeRes terminate() override {
    throw std::runtime_error("Unimplemented method called");
    return ResumeRes::FAILURE;
  }
  ResumeRes reset(ResetType EMBDEBUG_ATTR_UNUSED type) override {
    throw std::runtime_error("Unimplemented method called");
    return ResumeRes::FAILURE;
  }

  uint64_t getCycleCount() const override {
    throw std::runtime_error("Unimplemented method called");
    return 0;
  }
  uint64_t getInstrCount() const override {
    throw std::runtime_error("Unimplemented method called");
    return 0;
  }
  int getRegisterCount() const override {
    throw std::runtime_error("Unimplemented method called");
    return 0;
  }
  int getRegisterSize() const override {
    throw std::runtime_error("Unimplemented method called");
    return 0;
  }
  bool getSyscallArgLocs(
      SyscallArgLoc EMBDEBUG_ATTR_UNUSED &syscallIDLoc,
      std::vector<SyscallArgLoc> EMBDEBUG_ATTR_UNUSED &syscallArgLocs,
      SyscallArgLoc EMBDEBUG_ATTR_UNUSED &syscallReturnLoc) const override {
    throw std::runtime_error("Unimplemented method called");
    return 0;
  }
  std::size_t readRegister(const int EMBDEBUG_ATTR_UNUSED reg,
                           uint_reg_t EMBDEBUG_ATTR_UNUSED &value) override {
    throw std::runtime_error("Unimplemented method called");
    return 0;
  }
  std::size_t
  writeRegister(const int EMBDEBUG_ATTR_UNUSED reg,
                const uint_reg_t EMBDEBUG_ATTR_UNUSED value) override {
    throw std::runtime_error("Unimplemented method called");
    return 0;
  }
  std::size_t read(const uint_addr_t EMBDEBUG_ATTR_UNUSED addr,
                   uint8_t EMBDEBUG_ATTR_UNUSED *buffer,
                   const std::size_t EMBDEBUG_ATTR_UNUSED size) override {
    throw std::runtime_error("Unimplemented method called");
    return 0;
  }
  std::size_t write(const uint_addr_t EMBDEBUG_ATTR_UNUSED addr,
                    const uint8_t EMBDEBUG_ATTR_UNUSED *buffer,
                    const std::size_t EMBDEBUG_ATTR_UNUSED size) override {
    throw std::runtime_error("Unimplemented method called");
    return 0;
  }
  bool
  insertMatchpoint(const uint_addr_t EMBDEBUG_ATTR_UNUSED addr,
                   const MatchType EMBDEBUG_ATTR_UNUSED matchType) override {
    throw std::runtime_error("Unimplemented method called");
    return false;
  }
  bool
  removeMatchpoint(const uint_addr_t EMBDEBUG_ATTR_UNUSED addr,
                   const MatchType EMBDEBUG_ATTR_UNUSED matchType) override {
    throw std::runtime_error("Unimplemented method called");
    return false;
  }
  bool command(const std::string EMBDEBUG_ATTR_UNUSED cmd,
               std::ostream EMBDEBUG_ATTR_UNUSED &stream) override {
    throw std::runtime_error("Unimplemented method called");
    return false;
  }
  double timeStamp() override {
    throw std::runtime_error("Unimplemented method called");
    return 0.0;
  }
  unsigned int getCpuCount() override {
    throw std::runtime_error("Unimplemented method called");
    return 0;
  }
  unsigned int getCurrentCpu() override {
    throw std::runtime_error("Unimplemented method called");
    return 0;
  }
  void setCurrentCpu(unsigned int EMBDEBUG_ATTR_UNUSED num) override {
    throw std::runtime_error("Unimplemented method called");
    return;
  }
  bool prepare(const std::vector<ITarget::ResumeType> EMBDEBUG_ATTR_UNUSED
                   &actions) override {
    throw std::runtime_error("Unimplemented method called");
    return false;
  }
  bool resume(void) override {
    throw std::runtime_error("Unimplemented method called");
    return false;
  }
  WaitRes wait(std::vector<ResumeRes> EMBDEBUG_ATTR_UNUSED &results) override {
    throw std::runtime_error("Unimplemented method called");
    return WaitRes::ERROR;
  }
  bool halt(void) override {
    throw std::runtime_error("Unimplemented method called");
    return false;
  }
};

#endif
