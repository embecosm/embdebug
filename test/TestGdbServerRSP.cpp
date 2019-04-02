#include <stdexcept>

#include "AbstractConnection.h"
#include "GdbServer.h"
#include "RspPacket.h"
#include "StubTarget.h"
#include "embdebug/Compat.h"
#include "embdebug/ITarget.h"

#include "gtest/gtest.h"

using namespace EmbDebug;

// We need a dummy AbstractConnection which we can use to populate a packet
// to be used by the GdbServer when GdbServer::rspClientRequest is called, and
// from which we can extract any output packets which
// GdbServer::rspClientRequest produces.
class TestConnection : public AbstractConnection {
public:
  TestConnection(TraceFlags *traceFlags)
      : AbstractConnection(traceFlags), mInPos(0), mInBuf(nullptr), mOutPos(0),
        mOutBuf(nullptr) {}
  ~TestConnection() override {}

  bool rspConnect() override { return true; }
  void rspClose() override {}
  bool isConnected() override { return true; }

  void setInBuf(const char *buf) {
    mInBuf = buf;
    mInPos = 0;
  }
  void setOutBuf(char *buf) {
    mOutBuf = buf;
    mOutPos = 0;
  }

protected:
  bool putRspCharRaw(char c) override {
    if (!mOutBuf)
      throw std::runtime_error("Buffer accessed before being set");

    mOutBuf[mOutPos++] = c;
    mOutBuf[mOutPos] = '\0';
    return true;
  }
  int getRspCharRaw(bool EMBDEBUG_ATTR_UNUSED blocking) override {
    static bool endOfInput = false;
    if (!mInBuf)
      throw std::runtime_error("Buffer accessed before being set");
    if (endOfInput)
      throw std::runtime_error("Exhausted input packet data");

    char c = mInBuf[mInPos++];
    if (c == '\0')
      endOfInput = true;
    return c;
  }

private:
  size_t mInPos;
  const char *mInBuf;

  size_t mOutPos;
  char *mOutBuf;
};

// Test GdbServer. Normally the GdbServer only exposes the GdbServer::rspServer
// method publicly, which calls GdbServer::rspClientRequest to continually
// handle packets. In order to test the handling of packets individually, this
// class exposes the protected GdbServer::rspClientRequest method through the
// public method TestGdbServer::wrapRspClientRequest.
class TestGdbServer : public GdbServer {
public:
  TestGdbServer(AbstractConnection *conn, ITarget *cpu, TraceFlags *traceFlags,
                KillBehaviour killBehaviour)
      : GdbServer(conn, cpu, traceFlags, killBehaviour) {}
  ~TestGdbServer() {}

  void wrapRspClientRequest() { rspClientRequest(); }
};

// This is the absolute minimal target which can handle RSP packets.
class MinimalTestTarget : public StubTarget {
public:
  MinimalTestTarget(const TraceFlags *traceFlags) : StubTarget(traceFlags) {}
  ~MinimalTestTarget() override {}

  unsigned int getCpuCount() override { return 1; }

  int getRegisterCount() const override { return 1; }
};

typedef std::pair<std::string, std::string> MinimalTestCase;

class BasicRspTest : public ::testing::TestWithParam<MinimalTestCase> {
protected:
  void SetUp() override {
    flags = new TraceFlags();
    conn = new TestConnection(flags);
    target = new MinimalTestTarget(flags);
    server = new TestGdbServer(conn, target, flags, EXIT_ON_KILL);
  }
  void TearDown() override {
    delete server;
    delete target;
    delete conn;
    delete flags;
  }

  TraceFlags *flags;
  TestConnection *conn;
  MinimalTestTarget *target;
  TestGdbServer *server;
};

TEST_P(BasicRspTest, BasicRsp) {
  auto testCase = GetParam();

  char outBuf[1024];
  conn->setInBuf(testCase.first.c_str());
  conn->setOutBuf(outBuf);

  server->wrapRspClientRequest();

  EXPECT_EQ(std::strlen(outBuf), testCase.second.size());
  EXPECT_EQ(std::string(outBuf, outBuf + std::strlen(outBuf)), testCase.second);
}

INSTANTIATE_TEST_CASE_P(
    GdbServerRsp, BasicRspTest,
    ::testing::Values(
        MinimalTestCase("$!#21+", "+$OK#9a"),
        MinimalTestCase("$A#41+", "+$E01#a6"), MinimalTestCase("$b#62", "+"),
        MinimalTestCase("$B#42", "+"), MinimalTestCase("$c#63", "+"),
        MinimalTestCase("$C#43", "+"), MinimalTestCase("$d#64", "+"),
        MinimalTestCase("$k#6b", "+"), MinimalTestCase("$r#72", "+"),
        MinimalTestCase("$R#52", "+"), MinimalTestCase("$s#73", "+"),
        MinimalTestCase("$S#53", "+"), MinimalTestCase("$t#74", "+"),
        MinimalTestCase("$T#54+", "+$OK#9a"), MinimalTestCase("$L#4c", "+")));

struct RegisterReadWriteOp {
  bool mIsWrite;
  int mReg;
  uint_reg_t mValue;

  bool operator<(const RegisterReadWriteOp &other) const {
    return std::tie(mIsWrite, mReg, mValue) <
           std::tie(other.mIsWrite, other.mReg, other.mValue);
  }
  bool operator==(const RegisterReadWriteOp &other) const {
    return std::tie(mIsWrite, mReg, mValue) ==
           std::tie(other.mIsWrite, other.mReg, other.mValue);
  }
};

class RegisterReadWriteTestTarget : public StubTarget {
public:
  RegisterReadWriteTestTarget(const TraceFlags *traceFlags, int registerCount,
                              int registerSize)
      : StubTarget(traceFlags), mRegisterCount(registerCount),
        mRegisterSize(registerSize), mRegisterReadWriteOps() {}
  ~RegisterReadWriteTestTarget() override {}

  unsigned int getCpuCount() override { return 1; }

  int getRegisterCount() const override { return mRegisterCount; }
  int getRegisterSize() const override { return mRegisterSize; }

  std::size_t readRegister(const int reg, uint_reg_t &value) override {
    value = 0;
    RegisterReadWriteOp op = {false, reg, value};

    if (mRegisterReadWriteOps.count(op))
      throw std::runtime_error("Duplicate register read/write operation");
    mRegisterReadWriteOps.insert(op);
    return getRegisterSize();
  }

  std::size_t writeRegister(const int reg, const uint_reg_t value) override {
    RegisterReadWriteOp op = {true, reg, value};

    if (mRegisterReadWriteOps.count(op))
      throw std::runtime_error("Duplicate register read/write operation");
    mRegisterReadWriteOps.insert(op);
    return getRegisterSize();
  }

private:
  const int mRegisterCount;
  const int mRegisterSize;

public:
  std::set<RegisterReadWriteOp> mRegisterReadWriteOps;
};

struct RegisterReadWriteTestCase {
  int mRegCount;
  int mRegSize;
  std::string mInPacket;
  std::string mOutPacket;
  std::set<RegisterReadWriteOp> mRegisterReadWriteOps;
};

class RegisterReadWriteRspTest
    : public ::testing::TestWithParam<RegisterReadWriteTestCase> {
protected:
  void SetUp() override {
    auto testCase = GetParam();

    flags = new TraceFlags();
    conn = new TestConnection(flags);
    target = new RegisterReadWriteTestTarget(flags, testCase.mRegCount,
                                             testCase.mRegSize);
    server = new TestGdbServer(conn, target, flags, EXIT_ON_KILL);
  }
  void TearDown() override {
    delete server;
    delete target;
    delete conn;
    delete flags;
  }

  TraceFlags *flags;
  TestConnection *conn;
  RegisterReadWriteTestTarget *target;
  TestGdbServer *server;
};

TEST_P(RegisterReadWriteRspTest, RegisterReadWriteRsp) {
  auto testCase = GetParam();

  char outBuf[1024];
  conn->setInBuf(testCase.mInPacket.c_str());
  conn->setOutBuf(outBuf);

  server->wrapRspClientRequest();

  EXPECT_EQ(std::strlen(outBuf), testCase.mOutPacket.size());
  EXPECT_EQ(std::string(outBuf, outBuf + std::strlen(outBuf)),
            testCase.mOutPacket);
  EXPECT_EQ(target->mRegisterReadWriteOps, testCase.mRegisterReadWriteOps);
}

INSTANTIATE_TEST_CASE_P(
    GdbServerRsp, RegisterReadWriteRspTest,
    ::testing::Values(
        RegisterReadWriteTestCase(
            {1, 2, "$p0#a0+", "+$0000#c0", {{false, 0, 0}}}),
        RegisterReadWriteTestCase(
            {1, 2, "$P0=beef#4f+", "+$OK#9a", {{true, 0, 0xefbe}}}),
        RegisterReadWriteTestCase(
            {2, 2, "$g#67+", "+$00000000#80", {{false, 0, 0}, {false, 1, 0}}}),
        RegisterReadWriteTestCase({2,
                                   2,
                                   "$Gdeadbeef#67+",
                                   "+$OK#9a",
                                   {{true, 0, 0xadde}, {true, 1, 0xefbe}}})));

struct MemoryReadWriteOp {
  bool mIsWrite;
  uint_addr_t mAddr;
  std::vector<uint8_t> mValue;

  bool operator==(const MemoryReadWriteOp &other) const {
    return std::tie(mIsWrite, mAddr, mValue) ==
           std::tie(other.mIsWrite, other.mAddr, other.mValue);
  }
};

class MemoryReadWriteTestTarget : public StubTarget {
public:
  MemoryReadWriteTestTarget(const TraceFlags *traceFlags)
      : StubTarget(traceFlags), mMemoryReadWriteOp() {}
  ~MemoryReadWriteTestTarget() override {}

  unsigned int getCpuCount() override { return 1; }

  int getRegisterCount() const override { return 1; }

  std::size_t read(const uint_addr_t addr, uint8_t *buffer,
                   const std::size_t size) override {
    mMemoryReadWriteOp = {false, addr, std::vector<uint8_t>()};
    for (size_t i = 0; i < size; ++i)
      buffer[i] = 0;
    return size;
  }
  std::size_t write(const uint_addr_t addr, const uint8_t *buffer,
                    const std::size_t size) override {
    mMemoryReadWriteOp = {true, addr,
                          std::vector<uint8_t>(buffer, buffer + size)};
    return size;
  }

public:
  MemoryReadWriteOp mMemoryReadWriteOp;
};

struct MemoryReadWriteTestCase {
  std::string mInPacket;
  std::string mOutPacket;
  MemoryReadWriteOp mMemoryReadWriteOp;
};

class MemoryReadWriteRspTest
    : public ::testing::TestWithParam<MemoryReadWriteTestCase> {
protected:
  void SetUp() override {
    auto testCase = GetParam();

    flags = new TraceFlags();
    conn = new TestConnection(flags);
    target = new MemoryReadWriteTestTarget(flags);
    server = new TestGdbServer(conn, target, flags, EXIT_ON_KILL);
  }
  void TearDown() override {
    delete server;
    delete target;
    delete conn;
    delete flags;
  }

  TraceFlags *flags;
  TestConnection *conn;
  MemoryReadWriteTestTarget *target;
  TestGdbServer *server;
};

TEST_P(MemoryReadWriteRspTest, MemoryReadWriteRsp) {
  auto testCase = GetParam();

  char outBuf[1024];
  conn->setInBuf(testCase.mInPacket.c_str());
  conn->setOutBuf(outBuf);

  server->wrapRspClientRequest();

  EXPECT_EQ(std::strlen(outBuf), testCase.mOutPacket.size());
  EXPECT_EQ(std::string(outBuf, outBuf + std::strlen(outBuf)),
            testCase.mOutPacket);
  EXPECT_EQ(target->mMemoryReadWriteOp, testCase.mMemoryReadWriteOp);
}

INSTANTIATE_TEST_CASE_P(
    GdbServerRsp, MemoryReadWriteRspTest,
    ::testing::Values(MemoryReadWriteTestCase(
                          {"$mf00d,1:#2e+", "+$00#60", {false, 0xf00d, {}}}),
                      MemoryReadWriteTestCase({"$Mcabba9e5,1:55#0a+",
                                               "+$OK#9a",
                                               {true, 0xcabba9e5, {0x55}}})));
