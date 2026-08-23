# ARPG

Bullet hell RPG medieval-fantastique : carte roguelike, donjons bullet hell,
meta-progression entre les runs. Rendu pixel art basse resolution upscale au GPU.

Les choix techniques et leurs justifications sont dans [CLAUDE.md](CLAUDE.md).

## Stack

| Brique | Role |
| --- | --- |
| C++20 | langage |
| [raylib](https://github.com/raysan5/raylib) 5.5 | fenetre, rendu, audio, input, shaders |
| [EnTT](https://github.com/skypjack/entt) 3.16 | ECS et event dispatcher |
| [Dear ImGui](https://github.com/ocornut/imgui) + [rlImGui](https://github.com/raylib-extras/rlImGui) | outils de dev uniquement |
| [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) | dependances (vendore dans `cmake/`) |

Aucune dependance a installer a la main : CPM les telecharge a la configuration.

## Compiler

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/bin/arpg
```

Sous Windows avec MSVC, omettre `-G Ninja` et compiler avec
`cmake --build build --config Debug`.

Pour partager les sources telechargees entre plusieurs dossiers de build :

```sh
export CPM_SOURCE_CACHE=$HOME/.cache/CPM
```

### Prerequis systeme (Linux)

```sh
sudo apt install build-essential cmake ninja-build \
  libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libxi-dev libxkbcommon-dev libwayland-dev libasound2-dev
```

## Arborescence

```
src/core/     boucle de jeu, pile d'ecrans, canvas basse resolution
src/screens/  modes de jeu (menu, carte, donjon, pause)
src/ecs/      composants et systemes
src/ui/       widgets maison
assets/       textures, sons, shaders, data (JSON/TOML)
cmake/        CPM et declaration des dependances
```

## Raccourcis de dev

- `F1` : panneau de debug ImGui
- `ESC` : quitter (depuis le menu)
