
include_directories(${CMAKE_SOURCE_DIR}/include)

set(EMBDEBUG_SOURCES AbstractConnection.cpp
                     GdbServer.cpp
                     Init.cpp
                     ITarget.cpp
                     Ptid.cpp
                     RspPacket.cpp
                     StreamConnection.cpp
                     Timeout.cpp
                     TraceFlags.cpp
                     Utils.cpp
                     VContActions.cpp)
if (WIN32)
  list(APPEND EMBDEBUG_SOURCES RspConnectionWin32.cpp)
else()
  list(APPEND EMBDEBUG_SOURCES RspConnectionUnix.cpp)
endif()

# When building for Windows, link against winsock
if (WIN32)
  list(APPEND EMBDEBUG_LIBS ws2_32)
endif()

# Create embdebug server library
add_library(embdebug ${EMBDEBUG_SOURCES})
set_property(TARGET embdebug PROPERTY POSITION_INDEPENDENT_CODE 1)

if (BUILD_SHARED_LIBS)
  set_target_properties(embdebug PROPERTIES
                        VERSION ${embdebug_VERSION}
                        SOVERSION ${embdebug_VERSION_MAJOR})
else()
  set_target_properties(embdebug PROPERTIES
                        VERSION ${embdebug_VERSION})
endif()

target_link_libraries(embdebug ${EMBDEBUG_LIBS})
install(TARGETS embdebug
        DESTINATION lib)
