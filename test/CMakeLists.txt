include_directories(${CMAKE_SOURCE_DIR}/server)
include_directories(${CMAKE_SOURCE_DIR}/include)

set(TESTS TestAbstractConnection
          TestPtid
          TestRspPacket
          TestUtils)

foreach(_TEST ${TESTS})
  set (_SOURCES ${_TEST}.cpp)
  add_executable(${_TEST} ${_SOURCES})
  target_link_libraries(${_TEST} gtest gtest_main pthread embdebug)
  add_test(${_TEST} ${_TEST})
endforeach()
