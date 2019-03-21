cmake_minimum_required(VERSION 3.4.3)

set(embdebug_VERSION_MAJOR 0)
set(embdebug_VERSION_MINOR 0)
set(embdebug_VERSION_PATCH 0)
set(embdebug_VERSION
  ${embdebug_VERSION_MAJOR}.${embdebug_VERSION_MINOR}.${embdebug_VERSION_PATCH})

project(embdebug VERSION ${embdebug_VERSION} LANGUAGES C CXX)

# Setup the RPATH so that the dynamic linker can find the shared libraries when
# they are installed
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# Macro for testing whether a flag is supported, and adding it to CFLAGS/CXXFLAGS
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
macro (add_if_supported flag testname)
  check_c_compiler_flag(${flag} "C_SUPPORTS_${testname}")
  if(C_SUPPORTS_${testname})
    set(CMAKE_C_FLAGS "${flag} ${CMAKE_C_FLAGS}")
  endif()
  check_cxx_compiler_flag(${flag} "CXX_SUPPORTS_${testname}")
  if(CXX_SUPPORTS_${testname})
    set(CMAKE_CXX_FLAGS "${flag} ${CMAKE_CXX_FLAGS}")
  endif()
endmacro()

# Set C++ standard required and set warning flags
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set warning/error flags
option(EMBDEBUG_ENABLE_WERROR "Enable build failure if warnings triggered." OFF)
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  add_if_supported("/W4" "WARNINGS")
else()
  add_if_supported("-Wall -Wextra -pedantic" "WARNINGS")
endif()
if (EMBDEBUG_ENABLE_WERROR)
  if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_if_supported("/WX" "WERROR")
  else()
    add_if_supported("-Werror" "WERROR")
  endif()
endif()

enable_testing()

add_subdirectory(include/embdebug)
add_subdirectory(server)
add_subdirectory(tools)
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
                    COMMAND find tools -type f \\\( -name '*.c' -o -name '*.cpp' -o -name '*.h' \\\) -exec ${CLANG_FORMAT} -i {} \\\;
                    COMMAND find test -type f \\\( -name '*.c' -o -name '*.cpp' -o -name '*.h' \\\) -exec ${CLANG_FORMAT} -i {} \\\;
  )
endif()

message("
  " ${CMAKE_PROJECT_NAME} " version " ${embdebug_VERSION} "
  Prefix.....................: " ${CMAKE_INSTALL_PREFIX} "
  C Compiler.................: " ${CMAKE_C_COMPILER} "
  C++ Compiler...............: " ${CMAKE_CXX_COMPILER} "
  ")
