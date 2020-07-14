
include_directories(${CMAKE_SOURCE_DIR}/include)

set(TARGETLIB_SOURCES ITarget.cpp)

# Create embdebug server library
add_library(embdebugtarget ${TARGETLIB_SOURCES})
set_property(TARGET embdebugtarget PROPERTY POSITION_INDEPENDENT_CODE 1)

if (BUILD_SHARED_LIBS)
  set_target_properties(embdebugtarget PROPERTIES
                        VERSION ${embdebug_VERSION}
                        SOVERSION ${embdebug_VERSION_MAJOR})
else()
  set_target_properties(embdebugtarget PROPERTIES
                        VERSION ${embdebug_VERSION})
endif()

install(TARGETS embdebugtarget
        DESTINATION lib)
