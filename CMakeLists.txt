cmake_minimum_required(VERSION 3.21)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project(arpg
  VERSION 0.1.0
  DESCRIPTION "Bullet hell RPG med-fan"
  LANGUAGES C CXX
)

# --- Options globales ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Un seul dossier de sortie : l'exe trouve assets/ par un chemin stable.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Type de build" FORCE)
endif()

# --- Dependances (CPM) --------------------------------------------------------
include(CPM)
include(Dependencies)

# --- Cible principale ---------------------------------------------------------
add_executable(arpg
  src/main.cpp
  src/core/Application.cpp
  src/core/PixelCanvas.cpp
  src/core/ScreenManager.cpp
  src/screens/MainMenuScreen.cpp
)

target_include_directories(arpg PRIVATE src)
target_link_libraries(arpg PRIVATE raylib EnTT::EnTT rlimgui)

if(MSVC)
  target_compile_options(arpg PRIVATE /W4 /permissive- /utf-8)
else()
  target_compile_options(arpg PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Sur Windows, pas de console derriere le jeu en Release.
if(WIN32)
  set_target_properties(arpg PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
endif()

# assets/ copie a cote de l'exe a chaque build (les fichiers restent editables
# a chaud : c'est la copie qui est lue, on la rafraichit a chaque compilation).
add_custom_command(TARGET arpg POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/assets"
          "$<TARGET_FILE_DIR:arpg>/assets"
  COMMENT "Copie des assets"
)
