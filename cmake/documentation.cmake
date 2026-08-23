# SPDX-License-Identifier: BSL-1.0

if(NOT ARPG_BUILD_DOCUMENTATION)
  return()
endif()

find_package(Doxygen QUIET OPTIONAL_COMPONENTS dot)

if(NOT DOXYGEN_FOUND)
  message(WARNING "doxygen not found: the 'doc' target is unavailable")
  return()
endif()

# Stylesheet only, so it is fetched rather than configured as a project.
CPMAddPackage(
  NAME doxygen-awesome-css
  GITHUB_REPOSITORY jothepro/doxygen-awesome-css
  GIT_TAG v2.4.2
  DOWNLOAD_ONLY YES
)

set(ARPG_DOC_INPUT "${CMAKE_SOURCE_DIR}/src")
set(ARPG_DOC_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md")
set(ARPG_DOC_OUTPUT "${CMAKE_BINARY_DIR}/doc")
set(ARPG_DOC_AWESOME "${doxygen-awesome-css_SOURCE_DIR}")
set(ARPG_DOC_HAVE_DOT "NO")

if(TARGET Doxygen::dot)
  set(ARPG_DOC_HAVE_DOT "YES")
endif()

configure_file("${CMAKE_SOURCE_DIR}/doc/Doxyfile.in" "${CMAKE_BINARY_DIR}/Doxyfile" @ONLY)

add_custom_target(doc
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${ARPG_DOC_OUTPUT}"
  COMMAND Doxygen::doxygen "${CMAKE_BINARY_DIR}/Doxyfile"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  COMMENT "Generating the API documentation into ${ARPG_DOC_OUTPUT}"
  USES_TERMINAL
)
