include_directories(${CMAKE_SOURCE_DIR}/lib/server)
include_directories(${CMAKE_SOURCE_DIR}/include)

set(TESTS TestAbstractConnection
          TestPtid
          TestRspPacket
          TestUtils
          TestDebugServer)

# Supress a warning tripped in gtest
if (NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  add_if_supported("-Wno-gnu-zero-variadic-macro-arguments" "WNO_ZERO_MACRO_VARGS")
endif()

foreach(_TEST ${TESTS})
  set (_SOURCES ${_TEST}.cpp)
  add_executable(${_TEST} ${_SOURCES})
  target_link_libraries(${_TEST} gtest gtest_main embdebug embdebugtarget)
  add_test(${_TEST} ${_TEST})
endforeach()
