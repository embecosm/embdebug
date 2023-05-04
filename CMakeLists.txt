cmake_minimum_required(VERSION 3.4.3)

set(embdebug_VERSION_MAJOR 1)
set(embdebug_VERSION_MINOR 9)
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

# Generated header
configure_file(include/embdebug/config.h.in include/embdebug/config.h @ONLY)

add_subdirectory(include/embdebug)
add_subdirectory(lib/targetsupport)
add_subdirectory(lib/server)
add_subdirectory(targets)
add_subdirectory(tools)
add_subdirectory(test)
add_subdirectory(vendor)

enable_clang_format()

# Generate doxygen documentation
option(EMBDEBUG_ENABLE_DOXYGEN "Enable building of doxygen documentation" OFF)
if (EMBDEBUG_ENABLE_DOXYGEN)
  find_package(Doxygen)
  set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen")
  doxygen_add_docs(doxygen
                   include server docs)
endif()

# Generate documentation
option(EMBDEBUG_ENABLE_DOCS "Enable building of documentation" OFF)
if (EMBDEBUG_ENABLE_DOCS)
  find_program(SPHINX_BUILD "sphinx-build")
  if (NOT SPHINX_BUILD)
    message(STATUS "sphinx-build not found")
  else()
    set (SPHINX_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs/html")
    set (SPHINX_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/doc/embdebug/html")
    add_custom_target(docs
                      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/docs/html"
                      COMMAND ${SPHINX_BUILD}
                              -b html
                              "${CMAKE_CURRENT_SOURCE_DIR}/docs" # Source
                              "${SPHINX_BUILD_DIR}"              # Output
                      COMMENT "Generating html Sphinx documentation into \"${SPHINX_BUILD_DIR}\"")

    # Install to a typically location under share/doc/
    # The '/.' at the end means the contents will be copied without
    # implicitly recreating the last directory of ${SPHINX_BUILD_DIR}
    install(DIRECTORY "${SPHINX_BUILD_DIR}/."
            DESTINATION "${SPHINX_INSTALL_DIR}")
  endif()
endif()

message("
  " ${CMAKE_PROJECT_NAME} " version " ${embdebug_VERSION} "
  Prefix.....................: " ${CMAKE_INSTALL_PREFIX} "
  C Compiler.................: " ${CMAKE_C_COMPILER} "
  C++ Compiler...............: " ${CMAKE_CXX_COMPILER} "
  ")
