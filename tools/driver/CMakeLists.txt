include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/vendor)
include_directories(${CMAKE_SOURCE_DIR}/lib/server)

set(EMBDEBUG_DRIVER_SOURCES Driver.cpp)

add_executable(embdebug-driver ${EMBDEBUG_DRIVER_SOURCES})

if(WIN32)
  target_link_libraries(embdebug-driver embdebug embdebugtarget)
else()
  target_link_libraries(embdebug-driver embdebug embdebugtarget dl)
endif()

set_target_properties(embdebug-driver PROPERTIES
                      ENABLE_EXPORTS 1
                      OUTPUT_NAME embdebug)

install(TARGETS embdebug-driver DESTINATION bin)
