# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 3.21)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project(arpg
  VERSION 0.1.0
  DESCRIPTION "Bullet hell RPG med-fan"
  LANGUAGES C CXX
)

# --- Global options ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Single output directory, so the executable resolves assets/ from a stable path.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(ARPG_BUILD_TESTS "Build the unit tests" ON)
option(ARPG_ENABLE_COVERAGE "Instrument the code for gcov coverage" OFF)
option(ARPG_BUILD_DOCUMENTATION "Enable the doc target" OFF)

# --- Dependencies -----------------------------------------------------------
include(CPM)
include(dependencies)

# --- Flags shared by our own targets ----------------------------------------
add_library(arpg_warnings INTERFACE)
if(MSVC)
  target_compile_options(arpg_warnings INTERFACE /W4 /permissive- /utf-8)
else()
  target_compile_options(arpg_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

include(coverage)
include(documentation)

# --- Game logic -------------------------------------------------------------
# Everything here must stay free of raylib and of any other GUI call, so it can
# be unit tested without a window, a GPU context or an audio device.
add_library(arpg_core STATIC
  src/ecs/encounter.cpp
  src/ecs/enemy.cpp
  src/ecs/spatial_hash.cpp
  src/ecs/systems.cpp
  src/core/action_map.cpp
  src/core/camera.cpp
  src/core/action_state.cpp
  src/core/deadzone.cpp
  src/core/rng.cpp
  src/core/default_bindings.cpp
  src/core/screen_manager.cpp
  src/core/viewport.cpp
)

target_include_directories(arpg_core PUBLIC src)
target_link_libraries(arpg_core PUBLIC EnTT::EnTT arpg_coverage PRIVATE arpg_warnings)

# --- GUI layer --------------------------------------------------------------
# Kept as a library rather than folded into the executable so the compile tests
# can link it.
add_library(arpg_app STATIC
  src/core/application.cpp
  src/core/pixel_canvas.cpp
  src/core/raylib_input.cpp
  src/screens/dungeon_screen.cpp
  src/screens/main_menu_screen.cpp
)

target_link_libraries(arpg_app PUBLIC arpg_core raylib rlimgui PRIVATE arpg_warnings)

# --- Game executable --------------------------------------------------------
add_executable(arpg src/main.cpp)
target_link_libraries(arpg PRIVATE arpg_app arpg_warnings)

# A MinGW build links the GCC runtime dynamically, so the executable only starts
# where libgcc, libstdc++ and libwinpthread happen to be on the PATH: it fails
# on a double click and cannot be handed to anyone. Link them in instead.
if(WIN32 AND NOT MSVC)
  target_link_options(arpg PRIVATE -static)
endif()

# No console window behind the game in Release builds.
if(WIN32)
  set_target_properties(arpg PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)

  # The WINDOWS subsystem makes MSVC look for WinMain. MinGW ships a WinMain
  # that forwards to main, MSVC does not, so point it back at the usual entry.
  if(MSVC)
    target_link_options(arpg PRIVATE $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>)
  endif()
endif()

# assets/ is mirrored next to the executable on every build; that copy is the
# one the game reads and hot-reloads.
add_custom_command(TARGET arpg POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/assets"
          "$<TARGET_FILE_DIR:arpg>/assets"
  COMMENT "Mirroring assets"
)

# --- Tests ------------------------------------------------------------------
if(ARPG_BUILD_TESTS)
  # --error-exitcode makes valgrind fail the test it is checking; without it
  # ctest reports the defects and still exits zero, so a leak would pass the CI.
  # Only definite leaks count: possible ones are mostly reachable-at-exit noise.
  set(MEMORYCHECK_COMMAND_OPTIONS
      "--leak-check=full --errors-for-leak-kinds=definite --error-exitcode=1 --gen-suppressions=all")

  # CTest rather than enable_testing(), which is all it does beyond wiring the
  # memory check up.
  include(CTest)

  add_subdirectory(tests)
endif()
