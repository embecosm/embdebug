# Dummy target
include_directories(${CMAKE_SOURCE_DIR}/include)

set(DUMMY_SRCS dummytarget.cpp)
add_library (dummytarget SHARED ${DUMMY_SRCS})
set_target_properties(dummytarget PROPERTIES
                      VERSION 0.0.0
                      SOVERSION 0)

install(TARGETS dummytarget
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib)
