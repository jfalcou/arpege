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
tests/        tests unitaires (TTS), un executable par fichier
assets/       textures, sons, shaders, data (JSON/TOML)
cmake/        CPM et declaration des dependances
```

La cible `arpg_core` regroupe la logique sans aucun appel GUI et c'est elle que
les tests utilisent. Tout ce qui touche a raylib vit dans la cible `arpg`.

## Tests

```sh
ctest --test-dir build --output-on-failure
```

Deux familles de tests :

- **Tests unitaires** (TTS) sur la logique sans GUI. Un composant oriente rendu se
  teste avec une fausse sortie, comme le `fake_screen` de `tests/screen_stack.cpp`
  qui journalise les appels recus au lieu de dessiner.
- **Tests de compilation** (`tests/compile/`, sans framework) pour la couche GUI :
  le fichier decrit le cas d usage habituel de l API, et la seule assertion est
  qu il compile et s edite de liens. Rien n y est execute.

## Couverture

```sh
cmake -S . -B build-coverage -G Ninja -DCMAKE_BUILD_TYPE=Debug -DARPG_ENABLE_COVERAGE=ON
cmake --build build-coverage --target coverage
```

Produit un resume dans le terminal et un rapport HTML dans
`build-coverage/coverage/index.html`, plus un `cobertura.xml` pour la CI. Seule la
logique est mesuree : la couche GUI est exclue puisque les tests ne l executent pas.
Necessite gcc ou clang et `gcovr`.

## Formatage

Le style est fixe par `.clang-format` et applique par un hook `pre-commit`, a
installer une fois par clone :

```sh
pre-commit install
```

## Raccourcis de dev

- `F1` : panneau de debug ImGui
- `ESC` : quitter (depuis le menu)
