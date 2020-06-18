# Allow a user to specify which targets they wish to build, if no target is
# specified, build all targets
set (EMBDEBUG_TARGETS_TO_BUILD "" CACHE STRING "Specify targets to build")

if ("${EMBDEBUG_TARGETS_TO_BUILD}" STREQUAL "")
  message ("-- No targets specified, building all targets.")
  file (GLOB EMBDEBUG_TARGETS_TO_BUILD_tmp
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/*)
  set (EMBDEBUG_TARGETS_TO_BUILD "")
  foreach (t ${EMBDEBUG_TARGETS_TO_BUILD_tmp})
    if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${t}")
      list (APPEND EMBDEBUG_TARGETS_TO_BUILD ${t})
    endif()
  endforeach()
  unset (EMBDEBUG_TARGETS_TO_BUILD_tmp)
endif()

string(TOUPPER "${EMBDEBUG_TARGETS_TO_BUILD}" captilized_targets)
if ("${captilized_targets}" STREQUAL "NONE")
  message ("-- Building no targets.")
else()
foreach (t ${EMBDEBUG_TARGETS_TO_BUILD})
message ("-- Building target: ${t}")
  add_subdirectory (${t})
endforeach()
message ("-- Target configuration complete.")

endif()
