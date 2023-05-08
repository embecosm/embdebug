#include <stdexcept>

#include "embdebug/Compat.h"

#include "AbstractConnection.h"
#include "RspPacket.h"

#include "gtest/gtest.h"

using namespace EmbDebug;

// AbstractConnection has some pure virtual methods that we need to override to
// test it.
//
// Our TestConnection class accepts a buffer to draw characters from for testing
// purposes.

class TestConnection : public AbstractConnection {
public:
  TestConnection(TraceFlags *traceFlags)
      : AbstractConnection(traceFlags), _pos(0), _buf(nullptr) {}
  virtual ~TestConnection() override {}

  virtual bool rspConnect() override { return true; }
  virtual void rspClose() override {}
  virtual bool isConnected() override { return true; }

  void setBuf(const char *buf) {
    _buf = buf;
    _pos = 0;
  }

protected:
  virtual bool putRspCharRaw(char c EMBDEBUG_ATTR_UNUSED) override { /* TBC */
    return true;
  }
  virtual int getRspCharRaw(bool blocking EMBDEBUG_ATTR_UNUSED) override {
    if (_buf)
      return _buf[_pos++];
    else
      throw std::runtime_error("Buffer accessed before being set");
  }

private:
  size_t _pos;
  const char *_buf;
};

class AbstractConnectionTest : public ::testing::TestWithParam<std::string> {
protected:
  void SetUp() override {
    flags = new TraceFlags();
    tc = new TestConnection(flags);
    pkt = new RspPacket();
  }
  void TearDown() override {
    delete tc;
    delete flags;
    delete pkt;
  }

  TraceFlags *flags;
  TestConnection *tc;
  RspPacket *pkt;
};

std::string packetData(std::string buf) {
  return buf.substr(1, buf.size() - 4);
}

TEST_P(AbstractConnectionTest, GetPkt) {
  std::string buf = GetParam();
  tc->setBuf(buf.c_str());
  bool success;
  std::tie(success, *pkt) = tc->getPkt();
  EXPECT_TRUE(success);
  EXPECT_EQ(packetData(buf), pkt->getRawData());
}

// Problem: the AbstractConnection keeps reading until it gets a good checksum.
/*
TEST_P(AbstractConnectionTest, BadChecksum) {
  std::string buf = GetParam();
  // Change the packet so the checksum is invalid.
  buf[2] = 'a';
  tc->setBuf(buf.c_str());
  EXPECT_FALSE(tc->getPkt(pkt));
}
*/

// FIXME: Presently disabled, causes a segfault due to bad read of buffer.
TEST_P(AbstractConnectionTest, DISABLED_BufferOverrun) {
  std::string buf = GetParam();
  RspPacket tiny_pkt;
  tc->setBuf(buf.c_str());
  bool success;
  std::tie(success, tiny_pkt) = tc->getPkt();
  EXPECT_FALSE(success);
}

// PutPkt test is a work in progress.
/*
TEST_P(AbstractConnectionTest, PutPkt) {
  std::string inbuf = GetParam();
  const char* outbuf = calloc(sizeof(const char*) * 32);
  tc->setBuf(outbuf);
  EXPECT
  free(outbuf);
}*/

INSTANTIATE_TEST_SUITE_P(SimplePackets, AbstractConnectionTest,
                         ::testing::Values("$Hc-1#09", "$qOffsets#4b",
                                           "$p20#d2", "$qsThreadInfo#c8",
                                           "$P20=7601100100000000#ff",
                                           "$vCont;c:p1.-1#0f"));
