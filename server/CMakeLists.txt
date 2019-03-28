
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

# Create object library for building two server libraries
add_library(gdbserverobjs OBJECT ${EMBDEBUG_SOURCES})
set_property(TARGET gdbserverobjs PROPERTY POSITION_INDEPENDENT_CODE 1)
add_library(embdebug-shared SHARED $<TARGET_OBJECTS:gdbserverobjs>)
add_library(embdebug-static STATIC $<TARGET_OBJECTS:gdbserverobjs>)

if (BUILD_SHARED_LIBS)
  add_library(embdebug ALIAS embdebug-shared)
else()
  add_library(embdebug ALIAS embdebug-static)
endif()

set_target_properties(embdebug-shared PROPERTIES
                      VERSION ${embdebug_VERSION}
                      SOVERSION ${embdebug_VERSION_MAJOR})
set_target_properties(embdebug-static PROPERTIES
                      VERSION ${embdebug_VERSION})

install(TARGETS embdebug-shared embdebug-static
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
