# Allow a user to specify which targets they wish to build, if no target is
# specified, build all targets
option (EMBDEBUG_TARGETS_TO_BUILD "Specify targets to build")

if (NOT ${EMBDEBUG_TARGETS_TO_BUILD})
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

foreach (t ${EMBDEBUG_TARGETS_TO_BUILD})
message ("-- Building target: ${t}")
  add_subdirectory (${t})
endforeach()
message ("-- Target configuration complete.")
