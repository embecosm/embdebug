#include "RspPacket.h"

#include "gtest/gtest.h"

class RspPacketTest : public ::testing::Test {
protected:
  void SetUp() override {
    _pkt2 = new RspPacket(2);
    _pkt128 = new RspPacket(128);
  }

  void TearDown() override {
    delete _pkt2;
    delete _pkt128;
  }

  RspPacket *_pkt2;
  RspPacket *_pkt128;
};

TEST_F(RspPacketTest, GetBufSize) {
  EXPECT_EQ(2, _pkt2->getBufSize());
  EXPECT_EQ(128, _pkt128->getBufSize());
}

TEST_F(RspPacketTest, E01Message) {
  _pkt2->packStr("E01");
  EXPECT_EQ(std::string("E"), _pkt2->data);
  EXPECT_EQ(1, _pkt2->getLen());
  _pkt128->packStr("E01");
  EXPECT_EQ(std::string("E01"), _pkt128->data);
  EXPECT_EQ(3, _pkt128->getLen());
}

TEST_F(RspPacketTest, lMessage) {
  _pkt2->packStr("l");
  EXPECT_EQ(std::string("l"), _pkt2->data);
  EXPECT_EQ(1, _pkt2->getLen());
  _pkt128->packStr("l");
  EXPECT_EQ(std::string("l"), _pkt128->data);
  EXPECT_EQ(1, _pkt128->getLen());
}

TEST_F(RspPacketTest, vContMessage) {
  _pkt2->packStr("vCont;c;C;s;S");
  EXPECT_EQ(std::string("v"), _pkt2->data);
  EXPECT_EQ(1, _pkt2->getLen());
  _pkt128->packStr("vCont;c;C;s;S");
  EXPECT_EQ(std::string("vCont;c;C;s;S"), _pkt128->data);
  EXPECT_EQ(13, _pkt128->getLen());
}
