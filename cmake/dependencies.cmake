# SPDX-License-Identifier: BSL-1.0

include(quiet_dependency)

# Set the CPM_SOURCE_CACHE environment variable to share downloaded sources
# between build directories:
#   export CPM_SOURCE_CACHE=$HOME/.cache/CPM

# --- raylib -------------------------------------------------------------------
# raylib 5.5 vendors a GLFW whose cmake_minimum_required predates 3.5, which
# CMake 4 rejects. Lower the floor while configuring raylib, then restore it.
set(_arpg_policy_min_backup "${CMAKE_POLICY_VERSION_MINIMUM}")
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

CPMAddPackage(
  NAME raylib
  GITHUB_REPOSITORY raysan5/raylib
  GIT_TAG 5.5
  OPTIONS
    "BUILD_EXAMPLES OFF"
    "BUILD_SHARED_LIBS OFF"
    "USE_EXTERNAL_GLFW OFF"
)

set(CMAKE_POLICY_VERSION_MINIMUM "${_arpg_policy_min_backup}")

# --- EnTT ---------------------------------------------------------------------
CPMAddPackage(
  NAME EnTT
  GITHUB_REPOSITORY skypjack/entt
  GIT_TAG v3.16.0
)

# --- Dear ImGui ---------------------------------------------------------------
# No CMakeLists upstream, so the target is declared here.
CPMAddPackage(
  NAME imgui
  GITHUB_REPOSITORY ocornut/imgui
  GIT_TAG v1.92.9b-docking
  DOWNLOAD_ONLY YES
)

add_library(imgui STATIC
  "${imgui_SOURCE_DIR}/imgui.cpp"
  "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/imgui_demo.cpp"
)
target_include_directories(imgui SYSTEM PUBLIC "${imgui_SOURCE_DIR}")

# --- rlImGui ------------------------------------------------------------------
# No upstream tag and no CMakeLists: pinned to a commit and built here.
CPMAddPackage(
  NAME rlImGui
  GITHUB_REPOSITORY raylib-extras/rlImGui
  GIT_TAG db8239140a68bf9a07f4499d2875d7cefa8fa7e6
  DOWNLOAD_ONLY YES
)

add_library(rlimgui STATIC "${rlImGui_SOURCE_DIR}/rlImGui.cpp")
target_include_directories(rlimgui SYSTEM PUBLIC "${rlImGui_SOURCE_DIR}")
target_link_libraries(rlimgui PUBLIC imgui raylib)

# glfw is a target of its own inside raylib, and raylib compiles vendored
# libraries such as miniaudio and jar_mod that warn on their own.
foreach(_dep raylib glfw EnTT imgui rlimgui)
  arpg_quiet_dependency(${_dep})
endforeach()

# --- TTS ----------------------------------------------------------------------
# Header-only. Pulled without its own CMakeLists, which would drag in copacabana
# and build the tests of TTS itself.
if(ARPG_BUILD_TESTS)
  CPMAddPackage(
    NAME tts
    GITHUB_REPOSITORY jfalcou/tts
    GIT_TAG 3.0
    DOWNLOAD_ONLY YES
  )

  add_library(tts INTERFACE)
  target_include_directories(tts SYSTEM INTERFACE "${tts_SOURCE_DIR}/include")
  arpg_quiet_dependency(tts)
endif()
