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
    RESET,
    CYCLE_COUNT,
    INSTR_COUNT,
    PREPARE,
    RESUME,
    WAIT,
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

    struct ResetState {
      ITargetFunc func;
      ITarget::ResetType inType;
      ITarget::ResumeRes outRes;
    } resetState;

    struct CycleCountState {
      ITargetFunc func;
      uint64_t outValue;
    } cycleCountState;

    struct InstrCountState {
      ITargetFunc func;
      uint64_t outValue;
    } instrCountState;

    struct PrepareState {
      ITargetFunc func;
      ITarget::ResumeType inAction;
      bool outSuccess;
    } prepareState;

    struct ResumeState {
      ITargetFunc func;
      bool outSuccess;
    } resumeState;

    struct WaitState {
      ITargetFunc func;
      ITarget::ResumeRes outResumeResult;
      ITarget::WaitRes outWaitResult;
    } waitState;

    ITargetCall(const ReadRegisterState &other) : readRegisterState(other) {}
    ITargetCall(const WriteRegisterState &other) : writeRegisterState(other) {}
    ITargetCall(const ReadState &other) : readState(other) {}
    ITargetCall(const WriteState &other) : writeState(other) {}
    ITargetCall(const ResetState &other) : resetState(other) {}
    ITargetCall(const CycleCountState &other) : cycleCountState(other) {}
    ITargetCall(const InstrCountState &other) : instrCountState(other) {}
    ITargetCall(const PrepareState &other) : prepareState(other) {}
    ITargetCall(const ResumeState &other) : resumeState(other) {}
    ITargetCall(const WaitState &other) : waitState(other) {}
  };

  TraceTarget(const TraceFlags *traceFlags, int regCount, int regSize,
              std::vector<ITargetCall> targetTrace)
      : StubTarget(traceFlags), mRegisterCount(regCount),
        mRegisterSize(regSize), mHaveSyscallSupport(false),
        mITargetTrace(targetTrace), mITargetTracePos(mITargetTrace.begin()) {}

  TraceTarget(const TraceFlags *traceFlags, int regCount, int regSize,
              SyscallArgLoc syscallIDLoc,
              const std::vector<SyscallArgLoc> &syscallArgLocs,
              SyscallArgLoc syscallRetLoc, std::vector<ITargetCall> targetTrace)
      : StubTarget(traceFlags), mRegisterCount(regCount),
        mRegisterSize(regSize), mHaveSyscallSupport(true),
        mSyscallIDLoc(syscallIDLoc), mSyscallArgLocs(syscallArgLocs),
        mSyscallReturnLoc(syscallRetLoc), mITargetTrace(targetTrace),
        mITargetTracePos(mITargetTrace.begin()) {}

  ~TraceTarget() override {}

private:
  int mRegisterCount;
  int mRegisterSize;

  bool mHaveSyscallSupport;
  SyscallArgLoc mSyscallIDLoc;
  std::vector<SyscallArgLoc> mSyscallArgLocs;
  SyscallArgLoc mSyscallReturnLoc;

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
  bool command(const std::string EMBDEBUG_ATTR_UNUSED cmd,
               std::ostream EMBDEBUG_ATTR_UNUSED &stream) override {
    return false;
  }
  unsigned int getCpuCount() override { return 1; }

  int getRegisterCount() const override { return mRegisterCount; }
  int getRegisterSize() const override { return mRegisterSize; }

  bool getSyscallArgLocs(SyscallArgLoc &syscallIDLoc,
                         std::vector<SyscallArgLoc> &syscallArgLocs,
                         SyscallArgLoc &syscallReturnLoc) const override {
    auto *mutable_this = const_cast<TraceTarget *>(this);
    if (mHaveSyscallSupport) {
      syscallIDLoc = mutable_this->mSyscallIDLoc;
      syscallArgLocs = mutable_this->mSyscallArgLocs;
      syscallReturnLoc = mutable_this->mSyscallReturnLoc;
      return true;
    } else {
      return false;
    }
  }

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

  ResumeRes reset(ResetType type) override {
    auto &call = popAndVerifyCall(ITargetFunc::RESET);
    if (type != call.resetState.inType)
      throw std::runtime_error("Argument mismatch");
    return call.resetState.outRes;
  }

  uint64_t getCycleCount() const override {
    // Clumsy workaround - this is fine provided the underlying TraceTarget
    // is not declared constant.
    auto &call = const_cast<TraceTarget *>(this)->popAndVerifyCall(
        ITargetFunc::CYCLE_COUNT);
    return call.cycleCountState.outValue;
  }

  uint64_t getInstrCount() const override {
    // Clumsy workaround - this is fine provided the underlying TraceTarget
    // is not declared constant.
    auto &call = const_cast<TraceTarget *>(this)->popAndVerifyCall(
        ITargetFunc::INSTR_COUNT);
    return call.instrCountState.outValue;
  }

  void setCurrentCpu(unsigned int EMBDEBUG_ATTR_UNUSED index) override {}

  bool prepare(const std::vector<ResumeType> &actions) override {
    auto &call = popAndVerifyCall(ITargetFunc::PREPARE);
    if (actions.size() != 1 || actions[0] != call.prepareState.inAction) {
      throw std::runtime_error("Argument mismatch");
    }
    return call.prepareState.outSuccess;
  }

  bool resume(void) override {
    auto &call = popAndVerifyCall(ITargetFunc::RESUME);
    return call.resumeState.outSuccess;
  }

  WaitRes wait(std::vector<ResumeRes> &results) override {
    auto &call = popAndVerifyCall(ITargetFunc::WAIT);

    results.clear();
    results.resize(1);
    results[0] = call.waitState.outResumeResult;
    return call.waitState.outWaitResult;
  }

  bool supportsTargetXML(void) override { return true; }

  const char *getTargetXML(ByteView name) override {
    if (name == "target.xml")
      return "abcdefghijklmnopqrstuvwxyz";
    else
      return nullptr;
  }
};

struct GdbServerTestCase {
  int TargetRegisterSize;
  int TargetRegisterCount;

  bool HaveSyscallSupport;
  ITarget::SyscallArgLoc SyscallIDLoc;
  std::vector<ITarget::SyscallArgLoc> SyscallArgLocs;
  ITarget::SyscallArgLoc SyscallReturnLoc;

  std::string InStream;
  std::string ExpectedOutStream;
  std::vector<TraceTarget::ITargetCall> ITargetTrace;

  GdbServerTestCase(std::string inStream, std::string expectedOutStream,
                    const std::vector<TraceTarget::ITargetCall> &targetTrace)
      : TargetRegisterSize(1), TargetRegisterCount(1),
        HaveSyscallSupport(false), InStream(inStream),
        ExpectedOutStream(expectedOutStream), ITargetTrace(targetTrace) {}

  GdbServerTestCase(int targetRegSize, int targetRegCount, std::string inStream,
                    std::string expectedOutStream,
                    const std::vector<TraceTarget::ITargetCall> &targetTrace)
      : TargetRegisterSize(targetRegSize), TargetRegisterCount(targetRegCount),
        HaveSyscallSupport(false), InStream(inStream),
        ExpectedOutStream(expectedOutStream), ITargetTrace(targetTrace) {}

  GdbServerTestCase(int targetRegSize, int targetRegCount,
                    ITarget::SyscallArgLoc syscallIDLoc,
                    const std::vector<ITarget::SyscallArgLoc> &syscallArgLocs,
                    ITarget::SyscallArgLoc syscallReturnLoc,
                    std::string inStream, std::string expectedOutStream,
                    const std::vector<TraceTarget::ITargetCall> &targetTrace)
      : TargetRegisterSize(targetRegSize), TargetRegisterCount(targetRegCount),
        HaveSyscallSupport(true), SyscallIDLoc(syscallIDLoc),
        SyscallArgLocs(syscallArgLocs), SyscallReturnLoc(syscallReturnLoc),
        InStream(inStream), ExpectedOutStream(expectedOutStream),
        ITargetTrace(targetTrace) {}
};

class GdbServerTest : public ::testing::TestWithParam<GdbServerTestCase> {
protected:
  void SetUp() override {
    auto testCase = GetParam();

    flags = new TraceFlags();
    conn = new TraceConnection(flags);
    if (testCase.HaveSyscallSupport) {
      target = new TraceTarget(
          flags, testCase.TargetRegisterSize, testCase.TargetRegisterCount,
          testCase.SyscallIDLoc, testCase.SyscallArgLocs,
          testCase.SyscallReturnLoc, testCase.ITargetTrace);
    } else {
      target =
          new TraceTarget(flags, testCase.TargetRegisterSize,
                          testCase.TargetRegisterCount, testCase.ITargetTrace);
    }
    server = new GdbServer(conn, target, flags, EXIT_ON_KILL);
  }
  void TearDown() override {
    delete server;
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

// Tests of basic RSP packets with simple behavior.
GdbServerTestCase testBasicRSPPackets[] = {
    {"$vKill;1#6e+", "+$OK#9a", {}},
    {"$!#21+$vKill;1#6e+", "+$OK#9a+$OK#9a", {}},
    {"$A#41+$vKill;1#6e+", "+$E01#a6+$OK#9a", {}},
    {"$b#62+$vKill;1#6e+", "+$#00+$OK#9a", {}},
    {"$B#42+$vKill;1#6e+", "+$#00+$OK#9a", {}},
    {"$d#64+$vKill;1#6e+", "+$#00+$OK#9a", {}},
    {"$k#6b+$vKill;1#6e+", "+$#00+$OK#9a", {}},
    {"$R#52+$vKill;1#6e+", "+$#00+$OK#9a", {}},
    {"$t#74+$vKill;1#6e+", "+$#00+$OK#9a", {}},
    {"$T#54+$vKill;1#6e+", "+$OK#9a+$OK#9a", {}},
    {"$L#4c+$vKill;1#6e+", "+$#00+$OK#9a", {}},
};

INSTANTIATE_TEST_CASE_P(BasicRSPTest, GdbServerTest,
                        ::testing::ValuesIn(testBasicRSPPackets));

// Test of register reads and writes
GdbServerTestCase testRegisterRead = {
    /*reg count*/ 16,
    /*reg size*/ 4,
    "$pa#d1+$vKill;1#6e+",
    "+$efbe0000#52+$OK#9a",
    {
        TraceTarget::ITargetCall::ReadRegisterState(
            {TraceTarget::ITargetFunc::READ_REGISTER, 10, 0xbeef, 4}),
    },
};
GdbServerTestCase testRegisterWrite = {
    /*reg count*/ 32,
    /*reg size*/ 4,
    "$P1f=091d00ac#46+$vKill;1#6e+",
    "+$OK#9a+$OK#9a",
    {
        TraceTarget::ITargetCall::WriteRegisterState(
            {TraceTarget::ITargetFunc::WRITE_REGISTER, 31, 0xac001d09, 4}),
    },
};
GdbServerTestCase testRegisterReadAll = {
    /*reg count*/ 4,
    /*reg size*/ 2,
    "$g#67+$vKill;1#6e+",
    "+$bbcae5a901c00710#78+$OK#9a",
    {
        TraceTarget::ITargetCall::ReadRegisterState(
            {TraceTarget::ITargetFunc::READ_REGISTER, 0, 0xcabb, 2}),
        TraceTarget::ITargetCall::ReadRegisterState(
            {TraceTarget::ITargetFunc::READ_REGISTER, 1, 0xa9e5, 2}),
        TraceTarget::ITargetCall::ReadRegisterState(
            {TraceTarget::ITargetFunc::READ_REGISTER, 2, 0xc001, 2}),
        TraceTarget::ITargetCall::ReadRegisterState(
            {TraceTarget::ITargetFunc::READ_REGISTER, 3, 0x1007, 2}),
    }};
GdbServerTestCase testRegisterWriteAll = {
    /*reg count*/ 6,
    /*reg size*/ 1,
    "$G000102030405#96+$vKill;1#6e+",
    "+$OK#9a+$OK#9a",
    {
        TraceTarget::ITargetCall::WriteRegisterState(
            {TraceTarget::ITargetFunc::WRITE_REGISTER, 0, 0x00, 1}),
        TraceTarget::ITargetCall::WriteRegisterState(
            {TraceTarget::ITargetFunc::WRITE_REGISTER, 1, 0x01, 1}),
        TraceTarget::ITargetCall::WriteRegisterState(
            {TraceTarget::ITargetFunc::WRITE_REGISTER, 2, 0x02, 1}),
        TraceTarget::ITargetCall::WriteRegisterState(
            {TraceTarget::ITargetFunc::WRITE_REGISTER, 3, 0x03, 1}),
        TraceTarget::ITargetCall::WriteRegisterState(
            {TraceTarget::ITargetFunc::WRITE_REGISTER, 4, 0x04, 1}),
        TraceTarget::ITargetCall::WriteRegisterState(
            {TraceTarget::ITargetFunc::WRITE_REGISTER, 5, 0x05, 1}),
    }};

INSTANTIATE_TEST_CASE_P(RegisterReadWriteRSPTest, GdbServerTest,
                        ::testing::Values(testRegisterRead, testRegisterWrite,
                                          testRegisterReadAll,
                                          testRegisterWriteAll));

// Tests of memory reads and writes
GdbServerTestCase testMemoryInvalidRead1 = {
    "$m1234#37+$vKill;1#6e+", "+$E01#a6+$OK#9a", {}};
GdbServerTestCase testMemoryInvalidRead2 = {
    "$m1234,#63+$vKill;1#6e+", "+$E01#a6+$OK#9a", {}};
GdbServerTestCase testMemoryInvalidRead3 = {
    "$mhello,32:#4c+$vKill;1#6e+", "+$E01#a6+$OK#9a", {}};
GdbServerTestCase testMemoryInvalidRead4 = {
    "$m0095,world:#c9+$vKill;1#6e+", "+$E01#a6+$OK#9a", {}};

GdbServerTestCase testMemoryInvalidWrite1 = {
    "$M777#f2+$vKill;1#6e+", "+$E01#a6+$OK#9a", {}};
GdbServerTestCase testMemoryInvalidWrite2 = {
    "$Mbb00,#9d+$vKill;1#6e+", "+$E01#a6+$OK#9a", {}};
GdbServerTestCase testMemoryInvalidWrite3 = {
    "$Mfail,32:#b4+$vKill;1#6e+", "+$E01#a6+$OK#9a", {}};
GdbServerTestCase testMemoryInvalidWrite4 = {
    "$M1000,fail:#10+$vKill;1#6e+", "+$E01#a6+$OK#9a", {}};

GdbServerTestCase testMemoryWriteBufferTooLong = {
    "$M2000,4:1122334455667788#f1+$vKill;1#6e+", "+$E01#a6+$OK#9a", {}};
GdbServerTestCase testMemoryWriteBufferTooShort = {
    "$M800,4:112233#ab+$vKill;1#6e+", "+$E01#a6+$OK#9a", {}};

GdbServerTestCase testMemoryRead = {
    "$m124,2#62+$vKill;1#6e+",
    "+$beef#92+$OK#9a",
    {
        TraceTarget::ITargetCall::ReadState({TraceTarget::ITargetFunc::READ,
                                             0x124, 2,
                                             (const uint8_t *)"\xbe\xef", 2}),
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
GdbServerTestCase testMemoryBinaryWrite = {
    "$X88,4:\x11\x22\x33\x44#0c+$vKill;1#6e+",
    "+$OK#9a+$OK#9a",
    {
        TraceTarget::ITargetCall::WriteState({
            TraceTarget::ITargetFunc::WRITE,
            0x88,
            (const uint8_t *)"\x11\x22\x33\x44",
            4,
            4,
        }),
    },
};

INSTANTIATE_TEST_CASE_P(
    MemoryReadWriteRSPTest, GdbServerTest,
    ::testing::Values(testMemoryInvalidRead1, testMemoryInvalidRead2,
                      testMemoryInvalidRead3, testMemoryInvalidRead4,
                      testMemoryInvalidWrite1, testMemoryInvalidWrite2,
                      testMemoryInvalidWrite3, testMemoryInvalidWrite4,
                      testMemoryWriteBufferTooLong,
                      testMemoryWriteBufferTooShort, testMemoryRead,
                      testMemoryWrite, testMemoryBinaryWrite));

// Tests of vCont packets - stepping and continuing the target
GdbServerTestCase testVContQuery = {
    "$vCont?#49+$vKill;1#6e+", "+$vCont;c;C;s;S#62+$OK#9a", {}};
GdbServerTestCase testVContStep1 = {
    "$vCont:s#b7+$vKill;1#6e+",
    "+$S05#b8+$OK#9a",
    {
        TraceTarget::ITargetCall::PrepareState(
            {TraceTarget::ITargetFunc::PREPARE, ITarget::ResumeType::STEP,
             true}),
        TraceTarget::ITargetCall::CycleCountState(
            {TraceTarget::ITargetFunc::CYCLE_COUNT, 1234}),
        TraceTarget::ITargetCall::ResumeState(
            {TraceTarget::ITargetFunc::RESUME, true}),
        TraceTarget::ITargetCall::WaitState({TraceTarget::ITargetFunc::WAIT,
                                             ITarget::ResumeRes::STEPPED,
                                             ITarget::WaitRes::EVENT_OCCURRED}),
    },
};
GdbServerTestCase testVContStep2 = {
    "$vCont;S#98+$vKill;1#6e+",
    "+$S05#b8+$OK#9a",
    {
        TraceTarget::ITargetCall::PrepareState(
            {TraceTarget::ITargetFunc::PREPARE, ITarget::ResumeType::STEP,
             true}),
        TraceTarget::ITargetCall::CycleCountState(
            {TraceTarget::ITargetFunc::CYCLE_COUNT, 1234}),
        TraceTarget::ITargetCall::ResumeState(
            {TraceTarget::ITargetFunc::RESUME, true}),
        TraceTarget::ITargetCall::WaitState({TraceTarget::ITargetFunc::WAIT,
                                             ITarget::ResumeRes::STEPPED,
                                             ITarget::WaitRes::EVENT_OCCURRED}),
    },
};
GdbServerTestCase testVContContinue1 = {
    "$vCont;c#a8+$vKill;1#6e+",
    "+$S05#b8+$OK#9a",
    {
        TraceTarget::ITargetCall::PrepareState(
            {TraceTarget::ITargetFunc::PREPARE, ITarget::ResumeType::CONTINUE,
             true}),
        TraceTarget::ITargetCall::CycleCountState(
            {TraceTarget::ITargetFunc::CYCLE_COUNT, 1234}),
        TraceTarget::ITargetCall::ResumeState(
            {TraceTarget::ITargetFunc::RESUME, true}),
        TraceTarget::ITargetCall::WaitState({TraceTarget::ITargetFunc::WAIT,
                                             ITarget::ResumeRes::INTERRUPTED,
                                             ITarget::WaitRes::EVENT_OCCURRED}),
    },
};
GdbServerTestCase testVContContinue2 = {
    "$vCont;C#88+$vKill;1#6e+",
    "+$S05#b8+$OK#9a",
    {
        TraceTarget::ITargetCall::PrepareState(
            {TraceTarget::ITargetFunc::PREPARE, ITarget::ResumeType::CONTINUE,
             true}),
        TraceTarget::ITargetCall::CycleCountState(
            {TraceTarget::ITargetFunc::CYCLE_COUNT, 1234}),
        TraceTarget::ITargetCall::ResumeState(
            {TraceTarget::ITargetFunc::RESUME, true}),
        TraceTarget::ITargetCall::WaitState({TraceTarget::ITargetFunc::WAIT,
                                             ITarget::ResumeRes::INTERRUPTED,
                                             ITarget::WaitRes::EVENT_OCCURRED}),
    },
};
// Test continue in this block by emulating as vCont packets
GdbServerTestCase testStep1 = {
    "$s#73+$vKill;1#6e+",
    "+$S05#b8+$OK#9a",
    {
        TraceTarget::ITargetCall::PrepareState(
            {TraceTarget::ITargetFunc::PREPARE, ITarget::ResumeType::STEP,
             true}),
        TraceTarget::ITargetCall::CycleCountState(
            {TraceTarget::ITargetFunc::CYCLE_COUNT, 1234}),
        TraceTarget::ITargetCall::ResumeState(
            {TraceTarget::ITargetFunc::RESUME, true}),
        TraceTarget::ITargetCall::WaitState({TraceTarget::ITargetFunc::WAIT,
                                             ITarget::ResumeRes::STEPPED,
                                             ITarget::WaitRes::EVENT_OCCURRED}),
    },
};
GdbServerTestCase testStep2 = {
    "$S#53+$vKill;1#6e+",
    "+$S05#b8+$OK#9a",
    {
        TraceTarget::ITargetCall::PrepareState(
            {TraceTarget::ITargetFunc::PREPARE, ITarget::ResumeType::STEP,
             true}),
        TraceTarget::ITargetCall::CycleCountState(
            {TraceTarget::ITargetFunc::CYCLE_COUNT, 1234}),
        TraceTarget::ITargetCall::ResumeState(
            {TraceTarget::ITargetFunc::RESUME, true}),
        TraceTarget::ITargetCall::WaitState({TraceTarget::ITargetFunc::WAIT,
                                             ITarget::ResumeRes::STEPPED,
                                             ITarget::WaitRes::EVENT_OCCURRED}),
    },
};
GdbServerTestCase testContinue1 = {
    "$c#63+$vKill;1#6e+",
    "+$S05#b8+$OK#9a",
    {
        TraceTarget::ITargetCall::PrepareState(
            {TraceTarget::ITargetFunc::PREPARE, ITarget::ResumeType::CONTINUE,
             true}),
        TraceTarget::ITargetCall::CycleCountState(
            {TraceTarget::ITargetFunc::CYCLE_COUNT, 1234}),
        TraceTarget::ITargetCall::ResumeState(
            {TraceTarget::ITargetFunc::RESUME, true}),
        TraceTarget::ITargetCall::WaitState({TraceTarget::ITargetFunc::WAIT,
                                             ITarget::ResumeRes::INTERRUPTED,
                                             ITarget::WaitRes::EVENT_OCCURRED}),
    },
};
GdbServerTestCase testContinue2 = {
    "$C#43+$vKill;1#6e+",
    "+$S05#b8+$OK#9a",
    {
        TraceTarget::ITargetCall::PrepareState(
            {TraceTarget::ITargetFunc::PREPARE, ITarget::ResumeType::CONTINUE,
             true}),
        TraceTarget::ITargetCall::CycleCountState(
            {TraceTarget::ITargetFunc::CYCLE_COUNT, 1234}),
        TraceTarget::ITargetCall::ResumeState(
            {TraceTarget::ITargetFunc::RESUME, true}),
        TraceTarget::ITargetCall::WaitState({TraceTarget::ITargetFunc::WAIT,
                                             ITarget::ResumeRes::INTERRUPTED,
                                             ITarget::WaitRes::EVENT_OCCURRED}),
    },
};

INSTANTIATE_TEST_CASE_P(RSPVContTest, GdbServerTest,
                        ::testing::Values(testVContQuery, testVContStep1,
                                          testVContStep2, testVContContinue1,
                                          testVContContinue2, testStep1,
                                          testStep2, testContinue1,
                                          testContinue2));

// Tests of syscall handling and the associated RSP communication
GdbServerTestCase testSyscallClose = {
    /*reg count*/ 32,
    /*reg size*/ 4,
    /* syscall id location */
    ITarget::SyscallArgLoc::RegisterLoc(
        {ITarget::SyscallArgLocType::REGISTER, 17}),
    /* syscall argument locations */
    {
        ITarget::SyscallArgLoc::RegisterLoc(
            {ITarget::SyscallArgLocType::REGISTER, 10}),
        ITarget::SyscallArgLoc::RegisterLoc(
            {ITarget::SyscallArgLocType::REGISTER, 11}),
        ITarget::SyscallArgLoc::RegisterLoc(
            {ITarget::SyscallArgLocType::REGISTER, 12}),
    },
    /* syscall return location */
    ITarget::SyscallArgLoc::RegisterLoc(
        {ITarget::SyscallArgLocType::REGISTER, 10}),
    /* test case */
    "$vCont;c#a8+$F0#76+$vKill;1#6e+",
    "+$Fclose,15#ee+$S05#b8+$OK#9a",
    {
        TraceTarget::ITargetCall::PrepareState(
            {TraceTarget::ITargetFunc::PREPARE, ITarget::ResumeType::CONTINUE,
             true}),
        TraceTarget::ITargetCall::CycleCountState(
            {TraceTarget::ITargetFunc::CYCLE_COUNT, 1234}),
        TraceTarget::ITargetCall::ResumeState(
            {TraceTarget::ITargetFunc::RESUME, true}),
        TraceTarget::ITargetCall::WaitState({TraceTarget::ITargetFunc::WAIT,
                                             ITarget::ResumeRes::SYSCALL,
                                             ITarget::WaitRes::EVENT_OCCURRED}),

        TraceTarget::ITargetCall::ReadRegisterState(
            {TraceTarget::ITargetFunc::READ_REGISTER, 17, /*Fclose*/ 57, 4}),
        TraceTarget::ITargetCall::ReadRegisterState(
            {TraceTarget::ITargetFunc::READ_REGISTER, 10, 0x15, 4}),

        // Write result
        TraceTarget::ITargetCall::WriteRegisterState(
            {TraceTarget::ITargetFunc::WRITE_REGISTER, 10, 0, 4}),

        TraceTarget::ITargetCall::CycleCountState(
            {TraceTarget::ITargetFunc::CYCLE_COUNT, 1234}),
        TraceTarget::ITargetCall::ResumeState(
            {TraceTarget::ITargetFunc::RESUME, true}),
        TraceTarget::ITargetCall::WaitState({TraceTarget::ITargetFunc::WAIT,
                                             ITarget::ResumeRes::INTERRUPTED,
                                             ITarget::WaitRes::EVENT_OCCURRED}),
    },
};
GdbServerTestCase testSyscallOpen = {
    /*reg count*/ 32,
    /*reg size*/ 4,
    /* syscall id location */
    ITarget::SyscallArgLoc::RegisterLoc(
        {ITarget::SyscallArgLocType::REGISTER, 17}),
    /* syscall argument locations */
    {
        ITarget::SyscallArgLoc::RegisterLoc(
            {ITarget::SyscallArgLocType::REGISTER, 10}),
        ITarget::SyscallArgLoc::RegisterLoc(
            {ITarget::SyscallArgLocType::REGISTER, 11}),
        ITarget::SyscallArgLoc::RegisterLoc(
            {ITarget::SyscallArgLocType::REGISTER, 12}),
    },
    /* syscall return location */
    ITarget::SyscallArgLoc::RegisterLoc(
        {ITarget::SyscallArgLocType::REGISTER, 10}),
    /* test case */
    "$vCont;c#a8+$F0#76+$vKill;1#6e+",
    "+$Fopen,beef/5,0,0#d2+$S05#b8+$OK#9a",
    {
        TraceTarget::ITargetCall::PrepareState(
            {TraceTarget::ITargetFunc::PREPARE, ITarget::ResumeType::CONTINUE,
             true}),
        TraceTarget::ITargetCall::CycleCountState(
            {TraceTarget::ITargetFunc::CYCLE_COUNT, 1234}),
        TraceTarget::ITargetCall::ResumeState(
            {TraceTarget::ITargetFunc::RESUME, true}),
        TraceTarget::ITargetCall::WaitState({TraceTarget::ITargetFunc::WAIT,
                                             ITarget::ResumeRes::SYSCALL,
                                             ITarget::WaitRes::EVENT_OCCURRED}),

        TraceTarget::ITargetCall::ReadRegisterState(
            {TraceTarget::ITargetFunc::READ_REGISTER, 17, /*Fopen*/ 1024, 4}),
        TraceTarget::ITargetCall::ReadRegisterState(
            {TraceTarget::ITargetFunc::READ_REGISTER, 10, 0xbeef, 4}),
        TraceTarget::ITargetCall::ReadRegisterState(
            {TraceTarget::ITargetFunc::READ_REGISTER, 11, 0x0, 4}),
        TraceTarget::ITargetCall::ReadRegisterState(
            {TraceTarget::ITargetFunc::READ_REGISTER, 12, 0x0, 4}),

        // Read the path string "neat" from target memory (to get its length)
        TraceTarget::ITargetCall::ReadState({TraceTarget::ITargetFunc::READ,
                                             0xbeef, 1, (const uint8_t *)"n",
                                             1}),
        TraceTarget::ITargetCall::ReadState({TraceTarget::ITargetFunc::READ,
                                             0xbef0, 1, (const uint8_t *)"e",
                                             1}),
        TraceTarget::ITargetCall::ReadState({TraceTarget::ITargetFunc::READ,
                                             0xbef1, 1, (const uint8_t *)"a",
                                             1}),
        TraceTarget::ITargetCall::ReadState({TraceTarget::ITargetFunc::READ,
                                             0xbef2, 1, (const uint8_t *)"t",
                                             1}),
        TraceTarget::ITargetCall::ReadState({TraceTarget::ITargetFunc::READ,
                                             0xbef3, 1, (const uint8_t *)"\0",
                                             1}),

        // Write result
        TraceTarget::ITargetCall::WriteRegisterState(
            {TraceTarget::ITargetFunc::WRITE_REGISTER, 10, 0, 4}),

        TraceTarget::ITargetCall::CycleCountState(
            {TraceTarget::ITargetFunc::CYCLE_COUNT, 1234}),
        TraceTarget::ITargetCall::ResumeState(
            {TraceTarget::ITargetFunc::RESUME, true}),
        TraceTarget::ITargetCall::WaitState({TraceTarget::ITargetFunc::WAIT,
                                             ITarget::ResumeRes::INTERRUPTED,
                                             ITarget::WaitRes::EVENT_OCCURRED}),
    },
};
GdbServerTestCase testSyscallUnknown = {
    /*reg count*/ 32,
    /*reg size*/ 4,
    /* syscall id location */
    ITarget::SyscallArgLoc::RegisterLoc(
        {ITarget::SyscallArgLocType::REGISTER, 17}),
    /* syscall argument locations */
    {
        ITarget::SyscallArgLoc::RegisterLoc(
            {ITarget::SyscallArgLocType::REGISTER, 10}),
        ITarget::SyscallArgLoc::RegisterLoc(
            {ITarget::SyscallArgLocType::REGISTER, 11}),
        ITarget::SyscallArgLoc::RegisterLoc(
            {ITarget::SyscallArgLocType::REGISTER, 12}),
    },
    /* syscall return location */
    ITarget::SyscallArgLoc::RegisterLoc(
        {ITarget::SyscallArgLocType::REGISTER, 10}),
    /* test case */
    "$vCont;c#a8+$vKill;1#6e+",
    "+$S05#b8+$OK#9a",
    {
        TraceTarget::ITargetCall::PrepareState(
            {TraceTarget::ITargetFunc::PREPARE, ITarget::ResumeType::CONTINUE,
             true}),
        TraceTarget::ITargetCall::CycleCountState(
            {TraceTarget::ITargetFunc::CYCLE_COUNT, 1234}),
        TraceTarget::ITargetCall::ResumeState(
            {TraceTarget::ITargetFunc::RESUME, true}),
        TraceTarget::ITargetCall::WaitState({TraceTarget::ITargetFunc::WAIT,
                                             ITarget::ResumeRes::SYSCALL,
                                             ITarget::WaitRes::EVENT_OCCURRED}),

        TraceTarget::ITargetCall::ReadRegisterState(
            {TraceTarget::ITargetFunc::READ_REGISTER, 17, /*unknown*/ 666, 4}),

        // An exception is returned because the GdbServer cannot handle
        // the unknown syscall number
    },
};

INSTANTIATE_TEST_CASE_P(RSPSysCallTest, GdbServerTest,
                        ::testing::Values(testSyscallClose, testSyscallOpen,
                                          testSyscallUnknown));

// Tests of various qRcmd packets
GdbServerTestCase testCmdResetWarm = {
    "$qRcmd,7265736574#37+$vKill;1#6e+", // qRcmd,reset
    "+$OK#9a+$OK#9a",
    {
        TraceTarget::ITargetCall::ResetState({TraceTarget::ITargetFunc::RESET,
                                              ITarget::ResetType::WARM,
                                              ITarget::ResumeRes::SUCCESS}),
    },
};
GdbServerTestCase testCmdResetCold = {
    "$qRcmd,726573657420636f6c64#a1+$vKill;1#6e+", // qRcmd,reset cold
    "+$OK#9a+$OK#9a",
    {
        TraceTarget::ITargetCall::ResetState({TraceTarget::ITargetFunc::RESET,
                                              ITarget::ResetType::COLD,
                                              ITarget::ResumeRes::SUCCESS}),
    },
};
GdbServerTestCase testCmdExit = {
    "$qRcmd,65786974#d7", // qRcmd,exit
    "+",
    {},
};
GdbServerTestCase testCmdCycleCount = {
    "$qRcmd,6379636c65636f756e74#e0++$vKill;1#6e+", // cyclecount
    "+$O343636300a#7c$OK#9a+$OK#9a",                // 4660\n
    {
        TraceTarget::ITargetCall::CycleCountState(
            {TraceTarget::ITargetFunc::CYCLE_COUNT, 4660}),
    },
};
GdbServerTestCase testCmdInstrCount = {
    "$qRcmd,696e737472636f756e74#e2++$vKill;1#6e+", // instrcount
    "+$O3433393239383838380a#96$OK#9a+$OK#9a",      // 439298888\n
    {
        TraceTarget::ITargetCall::InstrCountState(
            {TraceTarget::ITargetFunc::INSTR_COUNT, 439298888}),
    },
};
GdbServerTestCase testCmdEcho = {
    // qRcmd,echo Hello World\n
    "$qRcmd,6563686f2048656c6c6f20576f726c640a#6f+$vKill;1#6e+",
    "+$OK#9a+$OK#9a",
    {},
};
GdbServerTestCase testCmdSetDebugInvalidFlag = {
    // qRcmd,set debug banana 1
    "$qRcmd,7365742064656275672062616e612031#d4+$vKill;1#6e+",
    "+$E01#a6+$OK#9a",
    {},
};
GdbServerTestCase testCmdShowDebugInvalidFlag = {
    // qRcmd,show debug banana
    "$qRcmd,73686f772064656275672062616e61#b0+$vKill;1#6e+",
    "+$E01#a6+$OK#9a",
    {},
};
GdbServerTestCase testCmdSetDebugFlagInvalidLevel = {
    // qRcmd,set debug rsp lemon
    "$qRcmd,73657420646562756720727370206c656d6f6e#ae+$vKill;1#6e+",
    "+$E02#a7+$OK#9a",
    {},
};
GdbServerTestCase testCmdSetAndShowDebugRspFlag = {
    // qRcmd,set debug rsp 1
    "$qRcmd,736574206465627567207273702031#3d"
    // qRcmd,show debug rsp
    "+$qRcmd,73686f7720646562756720727370#19"
    "++$vKill;1#6e+",
    // expected output rsp
    "+$OK#9a"
    "+$O7273703a204f4e0a#43$OK#9a" // rsp: ON\n
    "+$OK#9a",
    {},
};
GdbServerTestCase testCmdSetAndShowDebugConnFlag = {
    // qRcmd,set debug conn on
    "$qRcmd,73657420646562756720636f6e6e206f6e#11"
    // qRcmd,show debug conn
    "+$qRcmd,73686f7720646562756720636f6e6e#1a"
    "++$vKill;1#6e+",
    // expected output rsp
    "+$OK#9a"
    "+$O636f6e6e3a204f4e0a#44$OK#9a" // conn: ON\n
    "+$OK#9a",
    {},
};
GdbServerTestCase testCmdSetAndShowDebugDisasFlag = {
    // qRcmd,set debug disas FalSE
    "$qRcmd,7365742064656275672064697361732046616c5345#ee"
    // qRcmd,show debug disas
    "+$qRcmd,73686f77206465627567206469736173#f3"
    "++$vKill;1#6e+",
    // expected output rsp
    "+$OK#9a"
    // disas: OFF\n
    "+$O64697361733a204f46460a#58$OK#9a"
    "+$OK#9a",
    {},
};
GdbServerTestCase testCmdSetAndShowKillCoreOnExit = {
    // qRcmd,set kill-core-on-exit
    "$qRcmd,736574206b696c6c2d636f72652d6f6e2d65786974#84"
    // qRcmd,show kill-core-on-exit
    "+$qRcmd,73686f77206b696c6c2d636f72652d6f6e2d65786974#26"
    "++$vKill;1#6e+",
    // expected output rsp
    "+$OK#9a"
    // kill-core-on-exit: ON\n
    "+$O6b696c6c2d636f72652d6f6e2d657869743a204f4e0a#c8$OK#9a"
    "+$OK#9a",
    {},
};
GdbServerTestCase testCmdSetUnknownCommand = {
    // qRcmd,set unknown
    "$qRcmd,73657420756e6b6e6f776e#a4+$vKill;1#6e+",
    "+$E04#a9+$OK#9a",
    {},
};
GdbServerTestCase testCmdShowUnknownCommand = {
    // qRcmd,show unknown
    "$qRcmd,73686f7720756e6b6e6f776e#46+$vKill;1#6e+",
    "+$E04#a9+$OK#9a",
    {},
};

INSTANTIATE_TEST_CASE_P(
    RSPCmdPacketTest, GdbServerTest,
    ::testing::Values(
        testCmdResetWarm, testCmdResetCold, testCmdExit, testCmdCycleCount,
        testCmdInstrCount, testCmdEcho, testCmdSetDebugInvalidFlag,
        testCmdShowDebugInvalidFlag, testCmdSetDebugFlagInvalidLevel,
        testCmdSetAndShowDebugRspFlag, testCmdSetAndShowDebugConnFlag,
        testCmdSetAndShowDebugDisasFlag, testCmdSetAndShowKillCoreOnExit,
        testCmdSetUnknownCommand, testCmdShowUnknownCommand));

// Test of Target XML loading through ITarget
GdbServerTestCase testXMLWhole = {
    "$qXfer:features:read:target.xml:0,100#dc+$vKill;1#6e+",
    "+$labcdefghijklmnopqrstuvwxyz#8b+$OK#9a",
    {}};

GdbServerTestCase testXMLSplit = {
    "$qXfer:features:read:target.xml:0,10#ac+"
    "$qXfer:features:read:target.xml:10,10#dd+$vKill;1#6e+",
    "+$mabcdefghijklmnop#f5+$lqrstuvwxyz#03+$OK#9a",
    {}};

GdbServerTestCase testXMLInvalidName = {
    "$qXfer:features:read:nonexist.xml:0,100#cd+$vKill;1#6e+",
    "+$E00#a5+$OK#9a",
    {}};

INSTANTIATE_TEST_CASE_P(RSPXmlPacketTest, GdbServerTest,
                        ::testing::Values(testXMLWhole, testXMLSplit,
                                          testXMLInvalidName));
