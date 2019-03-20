cmake_minimum_required(VERSION 3.4.3)

set(embdebug_VERSION_MAJOR 0)
set(embdebug_VERSION_MINOR 0)
set(embdebug_VERSION_PATCH 0)
set(embdebug_VERSION
  ${embdebug_VERSION_MAJOR}.${embdebug_VERSION_MINOR}.${embdebug_VERSION_PATCH})

project(embdebug VERSION ${embdebug_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_FLAGS
  "-std=c++11 -Wall -Werror -Wextra -pedantic ${CMAKE_CXX_FLAGS}")

enable_testing()

add_subdirectory(include/embdebug)
add_subdirectory(server)
add_subdirectory(test)
add_subdirectory(vendor)

# Helper target for formatting source
find_program(CLANG_FORMAT "clang-format")
if(NOT CLANG_FORMAT)
  message(STATUS "clang-format not found")
else()
  message(STATUS "clang-format found: ${CLANG_FORMAT}. Enabling target clang-format")
  add_custom_target(clang-format
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    COMMAND find include/embdebug -type f \\\( -name '*.c' -o -name '*.cpp' -o -name '*.h' \\\) -exec ${CLANG_FORMAT} -i {} \\\;
                    COMMAND find server -type f \\\( -name '*.c' -o -name '*.cpp' -o -name '*.h' \\\) -exec ${CLANG_FORMAT} -i {} \\\;
                    COMMAND find test -type f \\\( -name '*.c' -o -name '*.cpp' -o -name '*.h' \\\) -exec ${CLANG_FORMAT} -i {} \\\;
  )
endif()

message("
  " ${CMAKE_PROJECT_NAME} " version " ${embdebug_VERSION} "
  Prefix.....................: " ${CMAKE_INSTALL_PREFIX} "
  C Compiler.................: " ${CMAKE_C_COMPILER} "
  C++ Compiler...............: " ${CMAKE_CXX_COMPILER} "
  ")
