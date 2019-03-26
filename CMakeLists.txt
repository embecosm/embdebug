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

# Set C/C++ standard required
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Configure compiler
option(EMBDEBUG_ENABLE_WERROR "Enable build failure if warnings triggered." OFF)
include(cmake/compiler_utils.cmake)
configure_compiler_defaults()

enable_testing()

add_subdirectory(include/embdebug)
add_subdirectory(server)
add_subdirectory(tools)
add_subdirectory(test)
add_subdirectory(vendor)

enable_clang_format()

# Generate doxygen documentation
option(EMBDEBUG_ENABLE_DOXYGEN "Enable building of doxygen documentation" OFF)
if (EMBDEBUG_ENABLE_DOXYGEN)
  find_package(Doxygen)
  doxygen_add_docs(doxygen
                   include server)
endif()

message("
  " ${CMAKE_PROJECT_NAME} " version " ${embdebug_VERSION} "
  Prefix.....................: " ${CMAKE_INSTALL_PREFIX} "
  C Compiler.................: " ${CMAKE_C_COMPILER} "
  C++ Compiler...............: " ${CMAKE_CXX_COMPILER} "
  ")
