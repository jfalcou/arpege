# SPDX-License-Identifier: BSL-1.0

include(quiet_dependency)

# Set the CPM_SOURCE_CACHE environment variable to share downloaded sources
# between build directories:
#   export CPM_SOURCE_CACHE=$HOME/.cache/CPM

# --- raylib -------------------------------------------------------------------
CPMAddPackage(
  NAME raylib
  GITHUB_REPOSITORY raysan5/raylib
  GIT_TAG 6.0
  OPTIONS
    "BUILD_EXAMPLES OFF"
    "BUILD_SHARED_LIBS OFF"
    "USE_EXTERNAL_GLFW OFF"
)

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

# --- Lua ----------------------------------------------------------------------
# The upstream mirror ships no CMakeLists. Sources are listed rather than
# globbed: the tree also holds lua.c and luac.c, which carry a main() each and
# would break the link.
CPMAddPackage(
  NAME lua
  GITHUB_REPOSITORY lua/lua
  GIT_TAG v5.4.8
  DOWNLOAD_ONLY YES
)

set(_lua_sources
  lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c
  lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c lundump.c
  lvm.c lzio.c
  lauxlib.c lbaselib.c lcorolib.c ldblib.c liolib.c lmathlib.c loadlib.c
  loslib.c lstrlib.c ltablib.c lutf8lib.c linit.c
)
list(TRANSFORM _lua_sources PREPEND "${lua_SOURCE_DIR}/")

add_library(lua STATIC ${_lua_sources})
target_include_directories(lua SYSTEM PUBLIC "${lua_SOURCE_DIR}")

# Lua is C, and compiling it as C++ changes how it reports errors: it would
# throw where the C build calls longjmp, which sol2 does not expect here.
set_target_properties(lua PROPERTIES LINKER_LANGUAGE C)

# --- sol2 ---------------------------------------------------------------------
# Header-only. Pulled without its own CMakeLists, which would go looking for a
# Lua of its own to find or build.
CPMAddPackage(
  NAME sol2
  GITHUB_REPOSITORY ThePhD/sol2
  GIT_TAG v3.5.0
  DOWNLOAD_ONLY YES
)

add_library(sol2 INTERFACE)
target_include_directories(sol2 SYSTEM INTERFACE "${sol2_SOURCE_DIR}/include")
target_link_libraries(sol2 INTERFACE lua)

# sol2 prefers <lua.hpp> whenever one can be found, and a distribution that
# ships its own Lua puts that header next to its own lua.h, which a quoted
# include then reaches before ours whatever the include order. MSYS2 ships a
# Lua 5.5 that way, and sol2 does not support it.
target_compile_definitions(sol2 INTERFACE SOL_NO_LUA_HPP=1)

foreach(_dep lua sol2)
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
