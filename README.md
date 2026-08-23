# Arpege

A top-down bullet hell RPG in a medieval fantasy setting. Runs are structured as
a roguelike map: you pick a route between nodes, clear bullet hell dungeons, and
carry gold and relics forward while unlocking new options between runs.

Everything is drawn as pixel art at a low internal resolution and upscaled by a
whole number, so the pixels stay square and sharp; the GPU is left to do the
lighting and the post-processing.

## Status

**Early — there is no game yet.** What exists is the engine foundation, and
running the executable opens a menu screen with a bouncing marker that responds
to the keyboard and to a gamepad. Nothing else is playable.

What the foundation covers:

- a fixed 60 Hz simulation step decoupled from the render rate, with the
  leftover of the accumulator handed to the renderer so movement interpolates
  smoothly at any refresh rate
- a stack of screens, where a pause can freeze the dungeon below without hiding
  it, and screen changes are applied at the end of a frame rather than in the
  middle of an update
- a low resolution canvas upscaled by an integer factor with letterboxing, and
  the mapping back from window coordinates to canvas pixels for aiming
- an input layer the gameplay talks to in terms of actions rather than keys, so
  remapping, AZERTY against QWERTY, keyboard, mouse and gamepad are all a matter
  of bindings; it does radial deadzones, eight-way movement, input buffering and
  tracks which device was last used
- an ImGui debug panel, on `F1`

What it does not cover yet: the dungeons, the enemies, the bullet patterns, the
run map, the audio, and saving.

## Built with

| Library | Used for |
| --- | --- |
| [raylib](https://github.com/raysan5/raylib) | window, rendering, audio, input, shaders |
| [EnTT](https://github.com/skypjack/entt) | entity component system and event dispatcher |
| [Dear ImGui](https://github.com/ocornut/imgui) + [rlImGui](https://github.com/raylib-extras/rlImGui) | developer tooling only, never the game UI |
| [TTS](https://github.com/jfalcou/tts) | unit tests |
| [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) | dependencies |

C++20, CMake 3.21 or later. Nothing has to be installed by hand: CPM downloads
every dependency at configure time.

## Building

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The executable lands in `build/bin`, next to a copy of `assets/`.

On Linux you also need the usual windowing development packages:

```sh
sudo apt install build-essential cmake ninja-build \
  libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libxi-dev libxkbcommon-dev libwayland-dev libasound2-dev
```

On Windows, both MSVC and MSYS2 UCRT64 are supported and covered by CI.

To share downloaded sources between build directories:

```sh
export CPM_SOURCE_CACHE=$HOME/.cache/CPM
```

## Controls

| Input | Action |
| --- | --- |
| `WASD`, arrows, left stick | move |
| `Esc`, gamepad east button | leave |
| `F1` | debug panel |

Both WASD and the arrow keys are bound, so the game is playable on an AZERTY
keyboard without touching anything.

## Layout

```
src/core/     game loop, screen stack, canvas, input layer
src/screens/  game modes
src/ecs/      components and systems
src/ui/       widgets
tests/        unit tests, and compile tests for what cannot be run headless
assets/       textures, sounds, shaders, data
cmake/        dependency declarations
```

The code is split in two targets on purpose. `arpg_core` holds the logic and
links no windowing library at all, which is what makes it testable; `arpg_app`
holds everything that touches raylib. A calculation that depends on GUI state
takes that state as a parameter instead of reaching for it, so it can be
exercised without a window.

## Development

```sh
ctest --test-dir build --output-on-failure
```

Tests come in two kinds. Unit tests cover the GUI-free logic, using a recording
double where a component is shaped around rendering. Compile tests cover the
rest: they spell out the usual usage of the GUI API and assert nothing beyond
building and linking, since that part cannot run without a window.

Coverage, which needs GCC or Clang and `gcovr`:

```sh
cmake -S . -B build-coverage -DCMAKE_BUILD_TYPE=Debug -DARPG_ENABLE_COVERAGE=ON
cmake --build build-coverage --target coverage
```

It prints a summary and writes an HTML report to `build-coverage/coverage`.

Formatting is settled by `.clang-format` and enforced by a hook, installed once
per clone:

```sh
pre-commit install
```

CI builds on Linux, MSVC and MSYS2 UCRT64, in Release and Debug, and runs the
tests, the coverage and the formatting check on every pull request.

## License

[Boost Software License 1.0](LICENSE). Every file carries an
`SPDX-License-Identifier: BSL-1.0` header, except `cmake/CPM.cmake`, a
third-party component that keeps its own MIT license.
