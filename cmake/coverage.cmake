# Coverage instrumentation, carried by an INTERFACE target so enabling it is a
# single link away. Left empty when the option is off.
add_library(arpg_coverage INTERFACE)

if(NOT ARPG_ENABLE_COVERAGE)
  return()
endif()

if(MSVC)
  message(FATAL_ERROR "ARPG_ENABLE_COVERAGE relies on gcov and needs GCC or Clang")
endif()

# -O0 keeps the line mapping honest; without it gcov reports lines that were
# folded away as never executed.
target_compile_options(arpg_coverage INTERFACE --coverage -O0 -g)
target_link_options(arpg_coverage INTERFACE --coverage)

find_program(GCOVR_EXECUTABLE NAMES gcovr)

if(NOT GCOVR_EXECUTABLE)
  message(WARNING "gcovr not found: the instrumentation is on but the 'coverage' target is unavailable")
  return()
endif()

set(_coverage_dir "${CMAKE_BINARY_DIR}/coverage")

# Runs the suite, then turns the .gcda files into a report. Only src/ is
# measured, and the GUI layer is excluded: it is never executed by the tests, so
# counting it would drown the figure that matters.
add_custom_target(coverage
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${_coverage_dir}"
  COMMAND "${CMAKE_CTEST_COMMAND}" --output-on-failure
  COMMAND "${GCOVR_EXECUTABLE}"
          --root "${CMAKE_SOURCE_DIR}"
          --filter "${CMAKE_SOURCE_DIR}/src/"
          --exclude "${CMAKE_SOURCE_DIR}/src/core/application.cpp"
          --exclude "${CMAKE_SOURCE_DIR}/src/core/pixel_canvas.cpp"
          --exclude "${CMAKE_SOURCE_DIR}/src/screens/"
          --exclude "${CMAKE_SOURCE_DIR}/src/main.cpp"
          --print-summary
          --html-details "${_coverage_dir}/index.html"
          --xml "${_coverage_dir}/cobertura.xml"
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Running the tests and collecting coverage into ${_coverage_dir}"
  USES_TERMINAL
)
