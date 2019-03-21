
include_directories(${CMAKE_SOURCE_DIR}/include)

set(EMBDEBUG_SOURCES AbstractConnection.cpp
                     GdbServer.cpp
                     Init.cpp
                     ITarget.cpp
                     MemMap.cpp
                     MpHash.cpp
                     Ptid.cpp
                     RspConnection.cpp
                     RspPacket.cpp
                     StreamConnection.cpp
                     Timeout.cpp
                     TraceFlags.cpp
                     Utils.cpp
                     VContActions.cpp)

add_library(embdebug SHARED ${EMBDEBUG_SOURCES})

set_target_properties(embdebug PROPERTIES
                      VERSION ${embdebug_VERSION}
                      SOVERSION ${embdebug_VERSION_MAJOR})

install(TARGETS embdebug DESTINATION lib)
