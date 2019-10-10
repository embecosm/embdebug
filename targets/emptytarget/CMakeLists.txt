# Empty target
include_directories(${CMAKE_SOURCE_DIR}/include)

set(EMPTY_SRCS emptytarget.cpp)
add_library (embdebug-target-empty SHARED ${EMPTY_SRCS})
set_target_properties(embdebug-target-empty PROPERTIES
                      VERSION 0.0.0
                      SOVERSION 0)

install(TARGETS embdebug-target-empty
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
