#include <stdexcept>

#include "AbstractConnection.h"
#include "GdbServer.h"
#include "RspPacket.h"
#include "StubTarget.h"
#include "embdebug/Compat.h"
#include "embdebug/ITarget.h"

#include "gtest/gtest.h"

using namespace EmbDebug;

class TraceConnection : public AbstractConnection {
public:
  TraceConnection(TraceFlags *traceFlags)
      : AbstractConnection(traceFlags), mInBuf(), mInBufPos(mInBuf.end()),
        mOutBuf() {}
  ~TraceConnection() override {}

  bool rspConnect() override { return true; }
  void rspClose() override {}
  bool isConnected() override { return true; }

  void setInBuf(std::string buf) {
    mInBuf = buf;
    mInBufPos = mInBuf.begin();
  }
  std::string getOutBuf() { return mOutBuf; }

protected:
  bool putRspCharRaw(char c) override {
    mOutBuf.push_back(c);
    return true;
  }
  int getRspCharRaw(bool EMBDEBUG_ATTR_UNUSED blocking) override {
    if (mInBufPos == mInBuf.end())
      throw std::runtime_error("Ran out of RSP input");

    return *(mInBufPos++);
  }

private:
  std::string mInBuf;
  std::string::iterator mInBufPos;

  std::string mOutBuf;
};

class TraceTarget : public StubTarget {
public:
  enum class ITargetFunc {
    READ_REGISTER,
    WRITE_REGISTER,
    READ,
    WRITE,
  };
  union ITargetCall {
    ITargetFunc func;

    struct ReadRegisterState {
      ITargetFunc func;
      int inReg;
      uint_reg_t outValue;
      std::size_t outSize;
    } readRegisterState;

    struct WriteRegisterState {
      ITargetFunc func;
      int inReg;
      uint_reg_t inValue;
      std::size_t outSize;
    } writeRegisterState;

    struct ReadState {
      ITargetFunc func;
      uint_addr_t inAddr;
      std::size_t inSize;
      const uint8_t *outBuffer;
      std::size_t outSize;
    } readState;

    struct WriteState {
      ITargetFunc func;
      uint_addr_t inAddr;
      const uint8_t *inBuffer;
      std::size_t inSize;
      std::size_t outSize;
    } writeState;

    ITargetCall(const ITargetCall::ReadState &readState)
        : readState(readState) {}
    ITargetCall(const ITargetCall::WriteState &writeState)
        : writeState(writeState) {}
  };

  TraceTarget(const TraceFlags *traceFlags,
              std::vector<ITargetCall> targetTrace)
      : StubTarget(traceFlags), mITargetTrace(targetTrace),
        mITargetTracePos(mITargetTrace.begin()) {}
  ~TraceTarget() override {}

private:
  std::vector<ITargetCall> mITargetTrace;
  std::vector<ITargetCall>::iterator mITargetTracePos;

  ITargetCall &popAndVerifyCall(ITargetFunc func) {
    if (mITargetTracePos == mITargetTrace.end())
      throw std::runtime_error("No more calls in ITarget trace");
    ITargetCall &call = *mITargetTracePos;
    ++mITargetTracePos;

    if (call.func != func)
      throw std::runtime_error("Function call mismatch");
    return call;
  }

public:
  unsigned int getCpuCount() override { return 1; }
  int getRegisterCount() const override { return 1; }

  std::size_t readRegister(const int reg, uint_reg_t &value) override {
    auto &call = popAndVerifyCall(ITargetFunc::READ_REGISTER);
    if (reg != call.readRegisterState.inReg)
      throw std::runtime_error("Argument mismatch");
    value = call.readRegisterState.outValue;
    return call.readRegisterState.outSize;
  }

  std::size_t writeRegister(const int reg, const uint_reg_t value) override {
    auto &call = popAndVerifyCall(ITargetFunc::WRITE_REGISTER);
    if (reg != call.writeRegisterState.inReg ||
        value != call.writeRegisterState.inValue) {
      throw std::runtime_error("Argument mismatch");
    }
    return call.writeRegisterState.outSize;
  }

  std::size_t read(const uint_addr_t addr, uint8_t *buffer,
                   const std::size_t size) override {
    auto &call = popAndVerifyCall(ITargetFunc::READ);
    if (addr != call.readState.inAddr || size != call.readState.inSize)
      throw std::runtime_error("Argument mismatch");

    for (std::size_t i = 0; i < call.readState.outSize; ++i)
      buffer[i] = call.readState.outBuffer[i];
    return call.readState.outSize;
  }

  std::size_t write(const uint_addr_t addr, const uint8_t *buffer,
                    const std::size_t size) override {
    auto &call = popAndVerifyCall(ITargetFunc::WRITE);
    if (addr != call.writeState.inAddr || size != call.writeState.inSize)
      throw std::runtime_error("Argument mismatch");
    for (std::size_t i = 0; i < call.writeState.inSize; ++i) {
      if (buffer[i] != call.writeState.inBuffer[i])
        throw std::runtime_error("Argument mismatch");
    }

    return call.writeState.outSize;
  }
};

struct GdbServerTestCase {
  std::string InStream;
  std::string ExpectedOutStream;

  std::vector<TraceTarget::ITargetCall> ITargetTrace;
};

class GdbServerTest : public ::testing::TestWithParam<GdbServerTestCase> {
protected:
  void SetUp() override {
    auto testCase = GetParam();

    flags = new TraceFlags();
    conn = new TraceConnection(flags);
    target = new TraceTarget(flags, testCase.ITargetTrace);
    server = new GdbServer(conn, target, flags, EXIT_ON_KILL);
  }
  void TearDown() override {
    delete flags;
    delete target;
    delete conn;
    delete flags;
  }

  TraceFlags *flags;
  TraceConnection *conn;
  TraceTarget *target;
  GdbServer *server;
};

TEST_P(GdbServerTest, GdbServerTest) {
  auto testCase = GetParam();

  conn->setInBuf(testCase.InStream);

  server->rspServer();

  auto OutStream = conn->getOutBuf();
  EXPECT_EQ(OutStream, testCase.ExpectedOutStream);
}

GdbServerTestCase testMemoryRead = {
    "$m124,2#62+$vKill;1#6e+",
    "+$beef#92+$OK#9a",
    {
        TraceTarget::ITargetCall::ReadState({TraceTarget::ITargetFunc::READ,
                                             0x124, 1, (const uint8_t *)"\xbe",
                                             1}),
        TraceTarget::ITargetCall::ReadState({TraceTarget::ITargetFunc::READ,
                                             0x125, 1, (const uint8_t *)"\xef",
                                             1}),
    },
};
GdbServerTestCase testMemoryWrite = {
    "$M9a7,1:4e#4e+$vKill;1#6e+",
    "+$OK#9a+$OK#9a",
    {
        TraceTarget::ITargetCall::WriteState({
            TraceTarget::ITargetFunc::WRITE,
            0x9a7,
            (const uint8_t *)"\x4e",
            1,
            1,
        }),
    },
};
INSTANTIATE_TEST_CASE_P(GdbServer, GdbServerTest,
                        ::testing::Values(testMemoryRead, testMemoryWrite));
