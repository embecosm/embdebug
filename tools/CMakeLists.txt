
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/vendor)

set(EMBDEBUG_DRIVER_SOURCES Driver.cpp)

add_executable(embdebug-driver ${EMBDEBUG_DRIVER_SOURCES})

target_link_libraries(embdebug-driver embdebug dl)

set_target_properties(embdebug-driver PROPERTIES
                      OUTPUT_NAME embdebug)

install(TARGETS embdebug-driver DESTINATION bin)
