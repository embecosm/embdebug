// Dummy ITarget interface implementation
//
// This file is part of the Embecosm GDB Server targets.
//
// Copyright (C) 2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include <sstream>

#include "embdebug/Compat.h"
#include "embdebug/ITarget.h"

#define PRINT_PLACEHOLDER()                                                    \
  std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << std::endl

using namespace EmbDebug;

class DummyTarget : public ITarget {
public:
  DummyTarget() = delete;
  DummyTarget(const DummyTarget &) = delete;

  explicit DummyTarget(const TraceFlags *traceFlags) : ITarget(traceFlags) {
    PRINT_PLACEHOLDER();
  }
  ~DummyTarget() { PRINT_PLACEHOLDER(); }

  ITarget::ResumeRes terminate() override {
    PRINT_PLACEHOLDER();
    return ResumeRes::SUCCESS;
  }
  ITarget::ResumeRes reset(ITarget::ResetType EMBDEBUG_ATTR_UNUSED) override {
    PRINT_PLACEHOLDER();
    return ResumeRes::SUCCESS;
  }
  uint64_t getCycleCount() const override {
    PRINT_PLACEHOLDER();
    return 0;
  }
  uint64_t getInstrCount() const override {
    PRINT_PLACEHOLDER();
    return 0;
  }
  int getRegisterCount() const override {
    PRINT_PLACEHOLDER();
    return 0;
  }
  int getRegisterSize() const override {
    PRINT_PLACEHOLDER();
    return 0;
  }
  std::size_t readRegister(const int reg EMBDEBUG_ATTR_UNUSED,
                           uint_reg_t &value EMBDEBUG_ATTR_UNUSED) override {
    PRINT_PLACEHOLDER();
    return 8;
  }
  std::size_t
  writeRegister(const int reg EMBDEBUG_ATTR_UNUSED,
                const uint_reg_t value EMBDEBUG_ATTR_UNUSED) override {
    PRINT_PLACEHOLDER();
    return 8;
  }
  std::size_t read(const uint_addr_t addr EMBDEBUG_ATTR_UNUSED,
                   uint8_t *buffer EMBDEBUG_ATTR_UNUSED,
                   const std::size_t size EMBDEBUG_ATTR_UNUSED) override {
    PRINT_PLACEHOLDER();
    return size;
  }
  std::size_t write(const uint_addr_t addr EMBDEBUG_ATTR_UNUSED,
                    const uint8_t *buffer EMBDEBUG_ATTR_UNUSED,
                    const std::size_t size EMBDEBUG_ATTR_UNUSED) override {
    PRINT_PLACEHOLDER();
    return size;
  }
  bool
  insertMatchpoint(const uint_addr_t addr EMBDEBUG_ATTR_UNUSED,
                   const MatchType matchType EMBDEBUG_ATTR_UNUSED) override {
    PRINT_PLACEHOLDER();
    return false;
  }
  bool
  removeMatchpoint(const uint_addr_t addr EMBDEBUG_ATTR_UNUSED,
                   const MatchType matchType EMBDEBUG_ATTR_UNUSED) override {
    PRINT_PLACEHOLDER();
    return false;
  }
  bool command(const std::string cmd EMBDEBUG_ATTR_UNUSED,
               std::ostream &stream EMBDEBUG_ATTR_UNUSED) override {
    PRINT_PLACEHOLDER();
    return false;
  }
  double timeStamp() override {
    PRINT_PLACEHOLDER();
    return 0.0;
  }
  unsigned int getCpuCount(void) override {
    PRINT_PLACEHOLDER();
    return 1;
  }
  unsigned int getCurrentCpu(void) override {
    PRINT_PLACEHOLDER();
    return 0;
  }
  void setCurrentCpu(unsigned int index EMBDEBUG_ATTR_UNUSED) override {
    PRINT_PLACEHOLDER();
  }
  bool prepare(
      const std::vector<ResumeType> &actions EMBDEBUG_ATTR_UNUSED) override {
    PRINT_PLACEHOLDER();
    return false;
  }
  bool resume(void) override {
    PRINT_PLACEHOLDER();
    return false;
  }
  WaitRes wait(std::vector<ResumeRes> &results EMBDEBUG_ATTR_UNUSED) override {
    PRINT_PLACEHOLDER();
    return WaitRes::ERROR;
  }
  bool halt(void) override {
    PRINT_PLACEHOLDER();
    return 0;
  }
};

// Entry point for the shared library
extern "C" {
EMBDEBUG_VISIBLE_API ITarget *create_target(TraceFlags *traceFlags) {
  return new DummyTarget(traceFlags);
}
EMBDEBUG_VISIBLE_API uint64_t ITargetVersion(void) {
  return ITarget::CURRENT_API_VERSION;
}
}
