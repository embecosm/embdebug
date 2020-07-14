# This file defines useful utilities and sets up compilers for the gdbserver

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

macro(configure_compiler_defaults)
  # Set warning/error flags
  if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_if_supported("/W4" "WARNINGS")
    set(CMAKE_CXX_FLAGS "-D_CRT_SECURE_NO_WARNINGS ${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "-D_CRT_SECURE_NO_WARNINGS ${CMAKE_C_FLAGS}")
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
endmacro()

# Helper target for formatting source
macro(enable_clang_format)
  find_program(CLANG_FORMAT "clang-format")
  if(NOT CLANG_FORMAT)
    message(STATUS "clang-format not found")
  else()
    message(STATUS "clang-format found: ${CLANG_FORMAT}. Enabling target clang-format")
    add_custom_target(clang-format
                      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                      COMMAND find include/embdebug -type f \\\( -name '*.c' -o -name '*.cpp' -o -name '*.h' \\\) -exec ${CLANG_FORMAT} -i {} \\\;
                      COMMAND find lib -type f \\\( -name '*.c' -o -name '*.cpp' -o -name '*.h' \\\) -exec ${CLANG_FORMAT} -i {} \\\;
                      COMMAND find tools -type f \\\( -name '*.c' -o -name '*.cpp' -o -name '*.h' \\\) -exec ${CLANG_FORMAT} -i {} \\\;
                      COMMAND find test -type f \\\( -name '*.c' -o -name '*.cpp' -o -name '*.h' \\\) -exec ${CLANG_FORMAT} -i {} \\\;
    )
  endif()
endmacro()
