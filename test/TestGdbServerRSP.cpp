#include <stdexcept>

#include "AbstractConnection.h"
#include "GdbServer.h"
#include "RspPacket.h"
#include "StubTarget.h"
#include "embdebug/Gdbserver_compat.h"
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
