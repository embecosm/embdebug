#include "RspPacket.h"

#include "gtest/gtest.h"

using namespace EmbDebug;

class RspPacketTest : public ::testing::Test {
protected:
  void SetUp() override { _pkt128 = new RspPacket(); }

  void TearDown() override { delete _pkt128; }

  RspPacket *_pkt128;
};

TEST_F(RspPacketTest, E01Message) {
  delete _pkt128;
  _pkt128 = new RspPacket("E01");
  EXPECT_EQ(std::string("E01"), _pkt128->getRawData());
  EXPECT_EQ(3, _pkt128->getLen());
}

TEST_F(RspPacketTest, lMessage) {
  delete _pkt128;
  _pkt128 = new RspPacket("l");
  EXPECT_EQ(std::string("l"), _pkt128->getRawData());
  EXPECT_EQ(1, _pkt128->getLen());
}

TEST_F(RspPacketTest, vContMessage) {
  delete _pkt128;
  _pkt128 = new RspPacket("vCont;c;C;s;S");
  EXPECT_EQ(std::string("vCont;c;C;s;S"), _pkt128->getRawData());
  EXPECT_EQ(13, _pkt128->getLen());
}
