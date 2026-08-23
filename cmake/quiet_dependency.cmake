# SPDX-License-Identifier: BSL-1.0

# Keeps a third-party target from polluting our warning output, on both fronts:
# its own sources are compiled without warnings, and its headers are marked as
# system headers so including them stays quiet on our side.
#
# Header-only targets have no sources to compile, so only the second half
# applies to them.
function(arpg_quiet_dependency name)
  if(NOT TARGET ${name})
    message(WARNING "arpg_quiet_dependency: no target named ${name}")
    return()
  endif()

  get_target_property(_type ${name} TYPE)

  if(NOT _type STREQUAL "INTERFACE_LIBRARY")
    if(MSVC)
      target_compile_options(${name} PRIVATE /W0)
    else()
      target_compile_options(${name} PRIVATE -w)
    endif()
  endif()

  # CPM adds most dependencies through add_subdirectory, which exposes their
  # include directories as normal ones. Restating them as system directories is
  # what -isystem does, without having to touch the dependency itself.
  get_target_property(_includes ${name} INTERFACE_INCLUDE_DIRECTORIES)

  if(_includes)
    set_target_properties(${name} PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_includes}")
  endif()
endfunction()
