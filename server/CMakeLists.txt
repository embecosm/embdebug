
include_directories(${CMAKE_SOURCE_DIR}/include)

set(EMBDEBUG_SOURCES AbstractConnection.cpp
                     GdbServer.cpp
                     Init.cpp
                     ITarget.cpp
                     MemMap.cpp
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

target_link_libraries(embdebug-shared ${EMBDEBUG_LIBS})
target_link_libraries(embdebug-static ${EMBDEBUG_LIBS})
install(TARGETS embdebug-shared embdebug-static
        DESTINATION lib)
