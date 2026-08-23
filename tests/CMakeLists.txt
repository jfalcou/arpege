# SPDX-License-Identifier: BSL-1.0

# Unit tests: one executable per file, TTS_MAIN provides a main() per file.
function(arpg_add_test name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE arpg_core tts arpg_warnings)
  add_test(NAME ${name} COMMAND ${name})

  # The coverage target runs ctest, which needs the executables built first.
  if(TARGET coverage)
    add_dependencies(coverage ${name})
  endif()
endfunction()

# Compile tests: the GUI layer cannot be exercised without a window, so the
# check is that its usual usage still builds and links. Nothing is run, and the
# test is the build command itself.
function(arpg_add_compile_test name)
  add_executable(compile_${name} EXCLUDE_FROM_ALL compile/${name}.cpp)
  target_link_libraries(compile_${name} PRIVATE arpg_app arpg_warnings)

  add_test(NAME compile_${name}
           COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}"
                   --target compile_${name} --config $<CONFIG>)

  # These tests drive the build tree, so they must not run concurrently with
  # each other.
  set_tests_properties(compile_${name} PROPERTIES RESOURCE_LOCK arpg_build_tree)
endfunction()

arpg_add_test(viewport)
arpg_add_test(screen_stack)

arpg_add_compile_test(screens)
arpg_add_compile_test(application)
