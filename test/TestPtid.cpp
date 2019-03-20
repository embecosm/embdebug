#include "Ptid.h"

#include "gtest/gtest.h"

/* Explicit instantiations of static members.  */
const int Ptid::PTID_ALL;
const int Ptid::PTID_INV;
const int Ptid::PTID_ANY;

TEST(PtidTest, Constructor) {
  Ptid *ptid;

  ptid = new Ptid(1, 1);
  EXPECT_EQ(1, ptid->pid());
  EXPECT_EQ(1, ptid->tid());
  delete ptid;

  ptid = new Ptid(2, 3);
  EXPECT_EQ(2, ptid->pid());
  EXPECT_EQ(3, ptid->tid());
  delete ptid;
}

TEST(PtidTest, TestSetters) {
  Ptid ptid(1,1);
  ptid.pid(4);
  EXPECT_EQ(4, ptid.pid());
  ptid.tid(5);
  EXPECT_EQ(5, ptid.tid());
}

TEST(PtidTest, TestDecodeTid) {
  /* Note: hex strings.  */
  Ptid ptid(6,7);
  EXPECT_TRUE(ptid.decode("8"));
  EXPECT_EQ(6, ptid.pid());
  EXPECT_EQ(8, ptid.tid());
  EXPECT_TRUE(ptid.decode("16"));
  EXPECT_EQ(6, ptid.pid());
  EXPECT_EQ(22, ptid.tid());
  EXPECT_TRUE(ptid.decode("FF"));
  EXPECT_EQ(6, ptid.pid());
  EXPECT_EQ(255, ptid.tid());
}

TEST(PtidTest, TestDecodePid) {
  /* Note: hex strings.  */
  Ptid ptid(1,1);
  EXPECT_TRUE(ptid.decode("p8"));
  EXPECT_EQ(8, ptid.pid());
  EXPECT_EQ(Ptid::PTID_ALL, ptid.tid());
  EXPECT_TRUE(ptid.decode("p16"));
  EXPECT_EQ(22, ptid.pid());
  EXPECT_EQ(Ptid::PTID_ALL, ptid.tid());
  EXPECT_TRUE(ptid.decode("pFF"));
  EXPECT_EQ(255, ptid.pid());
  EXPECT_EQ(Ptid::PTID_ALL, ptid.tid());
}

TEST(PtidTest, TestDecodePtid) {
  /* Note: hex strings.  */
  Ptid ptid(1,1);
  EXPECT_TRUE(ptid.decode("p8.3"));
  EXPECT_EQ(8, ptid.pid());
  EXPECT_EQ(3, ptid.tid());
  EXPECT_TRUE(ptid.decode("p16.20"));
  EXPECT_EQ(22, ptid.pid());
  EXPECT_EQ(32, ptid.tid());
  EXPECT_TRUE(ptid.decode("pFF.FE"));
  EXPECT_EQ(255, ptid.pid());
  EXPECT_EQ(254, ptid.tid());
}

TEST(PtidTest, TestInvalidDecode) {
  Ptid ptid(1,1);
  EXPECT_FALSE(ptid.decode("error"));
  EXPECT_EQ(1, ptid.pid());
  EXPECT_EQ(1, ptid.tid());
  EXPECT_FALSE(ptid.decode("perror"));
  EXPECT_EQ(1, ptid.pid());
  EXPECT_EQ(1, ptid.tid());
  EXPECT_FALSE(ptid.decode("p2.error"));
  EXPECT_EQ(1, ptid.pid());
  EXPECT_EQ(1, ptid.tid());
  EXPECT_FALSE(ptid.decode("perror.2"));
  EXPECT_EQ(1, ptid.pid());
  EXPECT_EQ(1, ptid.tid());
}

TEST(PtidTest, TestDecodeTidAll) {
  /* PTID_ALL seems to be considered a valid TID.  */
  Ptid ptid(1,1);
  EXPECT_TRUE(ptid.decode("-1"));
  EXPECT_EQ(Ptid::PTID_ALL, ptid.tid());
}

TEST(PtidTest, TestDecodePidTidAll) {
  /* PTID_ALL seems to be considered a valid TID.  */
  Ptid ptid(1,1);
  EXPECT_TRUE(ptid.decode("p3.-1"));
  EXPECT_EQ(3, ptid.pid());
  EXPECT_EQ(Ptid::PTID_ALL, ptid.tid());
}

TEST(PtidTest, TestDecodePidAllBad) {
  /* PTID_ALL is not a valid PID with no TID.  */
  Ptid ptid(1,1);
  EXPECT_FALSE(ptid.decode("p-1"));
  EXPECT_EQ(1, ptid.pid());
}

TEST(PtidTest, TestDecodePtidAllBad) {
  /* PTID_ALL is not valid for PID and TID.  */
  Ptid ptid(1,1);
  EXPECT_FALSE(ptid.decode("p-1.-1"));
  EXPECT_EQ(1, ptid.pid());
  EXPECT_EQ(1, ptid.tid());
}

TEST(PtidTest, TestValidate) {
  /* Note that validate is an unusual pattern - rather than accepting an
     inconsistent state then printing a warning later, why not just e.g. throw
     an exception when state would become invalid? This tests validate in its
     current form, anyway. Also, what exactly are the validation criteria?
     Needs clarifying / changing, perhaps.  */
  Ptid ptid1(-3,1);
  EXPECT_FALSE(ptid1.validate());
  Ptid ptid2(1, -3);
  EXPECT_FALSE(ptid2.validate());
}

TEST(PtidTest, Crystalize) {
  // A ptid with no abstract PID/TID components crystalizes to its existing
  // state.
  Ptid ptid1(1,1);
  EXPECT_TRUE(ptid1.crystalize(2, 3));
  EXPECT_EQ(1, ptid1.pid());
  EXPECT_EQ(1, ptid1.tid());

  // A ptid with ANY pid crystalizes to the default pid.
  Ptid ptid2(Ptid::PTID_ANY, 1);
  EXPECT_TRUE(ptid2.crystalize(2, 3));
  EXPECT_EQ(2, ptid2.pid());
  EXPECT_EQ(1, ptid2.tid());

  // A ptid with ANY tid crystalizes to the default pid.
  Ptid ptid3(1, Ptid::PTID_ANY);
  EXPECT_TRUE(ptid3.crystalize(2, 3));
  EXPECT_EQ(1, ptid3.pid());
  EXPECT_EQ(3, ptid3.tid());
}

TEST(PtidTest, BadCrystalize) {
  // A ptid with ALL or INV pid can't be crystalized, and its state doesn't
  // change.
  Ptid ptid1(Ptid::PTID_ALL, 1);
  EXPECT_FALSE(ptid1.crystalize(2, 3));
  EXPECT_EQ(Ptid::PTID_ALL, ptid1.pid());
  EXPECT_EQ(1, ptid1.tid());

  Ptid ptid2(Ptid::PTID_INV, 1);
  EXPECT_FALSE(ptid2.crystalize(2, 3));
  EXPECT_EQ(Ptid::PTID_INV, ptid2.pid());
  EXPECT_EQ(1, ptid1.tid());

  // A ptid with ALL or INV tid can't be crystalized, and its state doesn't
  // change.
  Ptid ptid3(1, Ptid::PTID_ALL);
  EXPECT_FALSE(ptid3.crystalize(2, 3));
  EXPECT_EQ(1, ptid3.pid());
  EXPECT_EQ(Ptid::PTID_ALL, ptid3.tid());

  Ptid ptid4(1, Ptid::PTID_INV);
  EXPECT_FALSE(ptid4.crystalize(2, 3));
  EXPECT_EQ(1, ptid4.pid());
  EXPECT_EQ(Ptid::PTID_INV, ptid4.tid());
}
