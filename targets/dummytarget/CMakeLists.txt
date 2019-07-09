# Dummy target
include_directories(${CMAKE_SOURCE_DIR}/include)

set(DUMMY_SRCS dummytarget.cpp)
add_library (embdebug-target-dummy SHARED ${DUMMY_SRCS})
set_target_properties(embdebug-target-dummy PROPERTIES
                      VERSION 0.0.0
                      SOVERSION 0)

install(TARGETS embdebug-target-dummy
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
