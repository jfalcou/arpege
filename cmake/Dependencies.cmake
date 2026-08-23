# Dependances du projet, gerees par CPM.cmake (vendore dans cmake/CPM.cmake).
#
# Definir CPM_SOURCE_CACHE (variable d'environnement) pour partager les sources
# telechargees entre plusieurs dossiers de build :
#   export CPM_SOURCE_CACHE=$HOME/.cache/CPM

# --- raylib -------------------------------------------------------------------
# raylib 5.5 embarque une version de GLFW dont le cmake_minimum_required est
# anterieur a 3.5, refuse par CMake >= 4. On abaisse le plancher le temps de
# configurer raylib, puis on restaure.
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

# --- EnTT (header-only : ECS + dispatcher) ------------------------------------
CPMAddPackage(
  NAME EnTT
  GITHUB_REPOSITORY skypjack/entt
  GIT_TAG v3.16.0
)

# --- Dear ImGui (outils de dev uniquement) ------------------------------------
# Pas de CMakeLists en amont : on compile nous-memes les sources.
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

# --- rlImGui (backend raylib pour ImGui) --------------------------------------
# Le depot n'a ni tag ni CMakeLists : on epingle un commit et on compile a la main.
CPMAddPackage(
  NAME rlImGui
  GITHUB_REPOSITORY raylib-extras/rlImGui
  GIT_TAG db8239140a68bf9a07f4499d2875d7cefa8fa7e6
  DOWNLOAD_ONLY YES
)

add_library(rlimgui STATIC "${rlImGui_SOURCE_DIR}/rlImGui.cpp")
target_include_directories(rlimgui SYSTEM PUBLIC "${rlImGui_SOURCE_DIR}")
target_link_libraries(rlimgui PUBLIC imgui raylib)

# Les dependances tierces ne doivent pas polluer nos warnings.
foreach(_dep imgui rlimgui)
  if(MSVC)
    target_compile_options(${_dep} PRIVATE /W0)
  else()
    target_compile_options(${_dep} PRIVATE -w)
  endif()
endforeach()
