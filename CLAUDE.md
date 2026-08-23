# Projet : Bullet Hell RPG médiéval-fantastique — Socle technique

## Vision
- Bullet hell vue de dessus, univers med-fan, avec phases roguelike d'amélioration (carte de type run → donjons bullet hell → retour carte).
- Pixel art 2D / isométrique max, mais exploitation du GPU pour les effets visuels (shaders).
- Priorités : développement rapide, efficace, **facilement éditable** (data-driven + hot-reload).

## Stack
- **C++20** minimum.
- **Raylib** (rendu, audio, input, shaders GLSL) + **EnTT** (ECS + event dispatcher) + **Dear ImGui** via `rlImGui` (outils de dev uniquement, pas l'UI du jeu).
- UI de jeu maison (boutons + navigation clavier/manette), esthétique pixel art.
- Plus tard si besoin : **sol2 + Lua** pour scripter les boss ; commencer en pur data (JSON/TOML).
- Cross-platform **Linux + Windows** : CMake + CPM.cmake, vendore dans `cmake/` (Raylib 6.0, EnTT 3.16, Dear ImGui + rlImGui). MSVC ou MinGW-w64 côté Windows. Rester sur `std::filesystem`, `std::thread`, `std::chrono` (pas d'API POSIX directes).
- CI (GitHub/Forgejo Actions) qui compile les deux cibles à chaque push, dès le début.

## Rendu pixel art
- Rendu du monde en basse résolution (ex. 320×180) dans une `RenderTexture2D`, puis passes de post-process (bloom, éclairage, color grading) et upscale final (nearest neighbor ou "sharp bilinear").
- Filtrage **nearest neighbor** partout.
- Éclairage 2D dynamique : lightmap dans une render texture séparée, mélange multiply/additive.
- Isométrique = 2D + tri des sprites par profondeur (sort sur Y/profondeur via ECS).
- Shaders GLSL = ressources rechargeables à chaud.

## Architecture des modes de jeu (Screens)
- Interface `screen` (virtual : `update(dt)`, `render()`, `on_enter/on_exit`, `blocks_update()`, `blocks_render()`) + `screen_manager` gérant une **pile**.
- `push()`/`pop()`/`replace()` : MainMenu → *replace* → RogueMap → *push* → DungeonScreen → *push* → PauseScreen. La carte roguelike survit intacte sous le donjon.
- Transitions (fade, wipe) = écrans spéciaux réutilisables.
- Changements d'écran mis en file, traités **en fin de frame** (jamais au milieu d'un update).
- Communication découplée : `entt::dispatcher` (event bus) ; les écrans ne s'appellent jamais entre eux.
- Chaque écran possède son monde : le donjon a son `entt::registry` + systèmes ; la carte a son graphe de nœuds ; le menu n'a pas d'ECS.
- Le donjon = fonction qui prend une `RunState` (référence partagée : or, reliques, PV, seed) et la modifie. Événements ponctuels (`DungeonClearedEvent`) via le dispatcher.
- Règle virtual : OK couche orchestration (écrans), **interdit** dans la boucle chaude ECS (entités = structs de données, systèmes = fonctions libres).

Arborescence :
```
src/
  core/     # screen_manager, événements, ressources, input, boucle
  screens/  # MainMenu, RogueMap, Dungeon, Pause, Upgrade...
  ecs/      # composants + systèmes
  ui/       # widgets maison réutilisables
assets/     # textures, sons, shaders, data (JSON/TOML)
```

## Inputs
- **Couche d'actions** : le gameplay consomme des actions (Dash, Move, Aim, Shoot, Confirm...), jamais des touches. Table de mapping clavier/manette → remapping et AZERTY/QWERTY triviaux.
- Move expose un `vec2` normalisé (clavier = 8 directions, stick = analogique).
- Deadzone **radiale** sur les sticks (pas par axe).
- Deux modes de visée : souris (position absolue) vs stick droit (direction relative).
- Détection du périphérique actif (timestamp du dernier input) pour afficher les bonnes icônes UI.
- **Input buffering** (quelques frames) dans la couche d'actions pour le feeling.
- Contextes d'actions par écran (menus : Navigate/Confirm/Cancel ; donjon : Move/Aim/Shoot/Dash).
- Capture des inputs 1×/frame de rendu, consommation dans les pas de simulation.

## Ressources
- **Resource Manager** centralisé : cache par clé (chargement unique), distribution par **handles** légers (jamais de copies).
- Durée de vie par écran : chaque écran déclare ses besoins (chargés à `on_enter`, libérables à `on_exit`) ; ressources globales permanentes. À cette échelle, tout garder en RAM est acceptable.
- **Atlas de textures** pour le batching (crucial avec des centaines de projectiles).
- Animations = données (rectangles d'atlas + durées, en fichier).
- **Data-driven** : ennemis, patterns, améliorations, salles = fichiers JSON/TOML référençant les ressources par nom. Ajouter du contenu sans recompiler.
- **Hot-reload** : surveillance des timestamps de fichiers (~1×/s), rechargement à la volée (textures, shaders, définitions).
- Empaquetage release (pack ou embarqué) : décision différée, l'abstraction par nom ne change pas.

## Boucle de jeu
- **Fixed timestep** : simulation à pas fixe (1/60 s) via accumulateur ; rendu à la fréquence de l'écran.
- **Interpolation de rendu** : position précédente + courante par entité, interpolation selon le reliquat de l'accumulateur.
- Déterminisme : même seed + mêmes inputs = même partie (replays, debug, équilibrage).
- Plafonner le nombre de pas par frame (anti spirale de la mort).
- dt variable autorisé pour le purement cosmétique (particules déco, transitions) ; tout ce qui affecte le gameplay est à pas fixe.

## Collisions
- Cercles uniquement (lasers = chapelets de cercles).
- Paires testées : balles ennemies↔joueur, tirs joueur↔ennemis, joueur↔ennemis/pickups. Jamais balle↔balle.
- Hitbox joueur minuscule (quelques pixels), affichable en mode focus.
- **Spatial hash** (grille uniforme reconstruite chaque frame) si les tests bruts ne suffisent plus. Pas de quadtree.
- Balles hors écran (+ marge) détruites immédiatement.

## Patterns de tir (data-driven)
- Pattern = composition de briques paramétrées :
  - **Émetteur** : position (fixe/attaché/orbitant), cadence, balles par salve.
  - **Distribution angulaire** : visé, éventail (N sur X°), cercle, spirale (angle incrémenté par salve).
  - **Comportement de balle** : vitesse, accélération, courbure, changement à mi-vie, éclatement en sous-balles.
  - **Modulation temporelle** : phases, pauses, (option : sync musique).
- Décrit en JSON/TOML + hot-reload → réglage des boss en direct.
- Boss complexes multi-phases : Lua/sol2 quand le besoin émerge.
- Balle (ECS) = position, vélocité, rayon, sprite, référence de comportement.

## Audio
- SFX chargés en mémoire (WAV/OGG) ; musique **streamée** (OGG, update 1×/frame).
- Anti-spam : max 3-4 instances simultanées par son, délai min ~50 ms entre déclenchements identiques, atténuation quand ça sature.
- **Pitch aléatoire ±5-10 %** sur les SFX répétés.
- Priorités : sons critiques (dégât joueur, bombe) passent toujours.
- 3-4 bus de volume : Master, Musique, SFX (+ UI).
- Sons = ressources nommées dans la data (`"sound": "shot_small"`).
- Déclenchement via event bus (le gameplay ignore l'audio).
- Musique par écran, transitions en cross-fade.
- Polish différé : ducking, musique par couches selon la phase du boss.
- Sources : freesound.org, itch.io, sfxr/jsfxr (8-bit).

## Sauvegarde & méta-progression
Trois couches d'état :
1. **Méta (permanente)** : monnaie méta, déblocages, stats, succès, options.
   - JSON lisible, champ `"version"` dès le jour 1 + migrations (défauts pour champs manquants).
   - **Écriture atomique** (fichier temp puis rename).
   - Emplacement : `~/.local/share/<jeu>` (Linux) / `%APPDATA%` (Windows).
2. **RunState (semi-persistante)** : or, reliques, PV, position carte, seed. Sauvegardée **entre les nœuds** (retour carte), jamais en plein donjon. Quitter en donjon = reprise au dernier nœud.
3. **État de combat : jamais sauvegardé** (se rejoue).
- Seed par run stockée dans la RunState : carte reproductible, daily runs, partage de seeds, repro de bugs. Séparer le RNG *contenu* (seedé) du RNG *cosmétique* (libre).
- Design méta : déblocages **horizontaux** (options, armes, personnages) plutôt que verticaux (+X % cumulables).

## Conventions de code

- Fichiers en **minuscules avec underscores** (`screen_manager.hpp`), symboles en **snake_case** — jamais de PascalCase ni de camelCase. Les API tierces gardent leur orthographe d origine.
- Style `.clang-format` : Allman, 2 espaces, 120 colonnes, applique par un hook `pre-commit`.
- Commentaires en **anglais**, et uniquement des explications techniques : pas de journal de décision, pas de paraphrase du code.
- Messages de commit en anglais.
- Licence **BSL-1.0** : en-tête `SPDX-License-Identifier: BSL-1.0` en première ligne de chaque fichier source, header, CMake et YAML. Les composants tiers vendorés gardent leur propre licence.

## Testabilité (règle d'architecture)

Le jeu est visuel, donc rien ne se teste « à l'écran » : **le maximum de logique doit vivre hors de toute dépendance GUI**, et chaque composant doit pouvoir tourner contre une fausse sortie.

- Cible **`arpg_core`** : logique pure, ne link ni raylib ni rlImGui. C'est la seule cible que les tests utilisent.
- Cible **`arpg_app`** : tout ce qui appelle raylib (fenêtre, textures, audio, input) — couche mince qui délègue à `arpg_core`. C est une bibliothèque et non l exécutable, pour que les tests de compilation puissent s y lier ; `arpg` se réduit à `main.cpp`.
- Quand un calcul dépend d'un état GUI (taille de fenêtre, position souris), il devient une **fonction pure prenant cet état en paramètre**. Exemple : `core/viewport` calcule échelle, letterbox et conversion fenêtre→canvas sans appeler raylib ; `pixel_canvas` se contente de lui passer `GetScreenWidth()`.
- Un composant orienté rendu se teste avec un **double qui enregistre au lieu de dessiner** (`fake_screen` dans `tests/screen_stack.cpp`).
- Ce qui ne peut pas être exécuté du tout reçoit un **test de compilation** (`tests/compile/`, sans framework) : le fichier décrit le cas d usage habituel, la seule assertion est qu il compile et s édite de liens, plus des `static_assert` sur les traits qui comptent. Branché dans CTest via la commande de build elle-même.
- **Couverture** via `-DARPG_ENABLE_COVERAGE=ON` et la cible `coverage` (gcov + gcovr) : résumé terminal, HTML et `cobertura.xml`. Mesure la logique seule, la couche GUI étant exclue.
- Tests avec **TTS** (`jfalcou/tts`), un exécutable par fichier, enregistrés dans CTest et lancés en CI sur les deux plateformes.

## État d'avancement

**Fait — socle technique** (branche `bootstrap`) :
- `CMakeLists.txt` + `cmake/dependencies.cmake` : raylib 6.0, EnTT 3.16, Dear ImGui 1.92.9b-docking, rlImGui (pas de tag en amont → épinglé au commit `db823914`), TTS 3.0. ImGui, rlImGui et TTS n'ayant pas de CMakeLists exploitable, leurs cibles sont déclarées chez nous.
- `core/application` : boucle à pas fixe 60 Hz (accumulateur, plafond de 5 pas/frame), `alpha` d'interpolation transmis au rendu.
- `core/screen_manager` : pile d'écrans, commandes appliquées en fin de frame, update/rendu s'arrêtant au premier écran bloquant. Sans dépendance GUI, donc testé.
- `core/viewport` : géométrie du canvas (échelle entière, letterbox, fenêtre→canvas) en fonctions pures, testée.
- **Couche d'actions** entièrement dans `arpg_core` : `action` (le vocabulaire du gameplay), `action_map` (bindings → actions, une table par contexte), `action_state` (fronts + buffering), `deadzone` (radiale), `vec2`, `default_bindings` (WASD + flèches + manette). Seul `core/raylib_input` touche raylib : il remplit un `input_snapshot` que tout le reste consomme.
- Échantillonnage **1× par frame de rendu** dans `application::sample_input`, consommé par chaque pas de simulation — c'est ce qui garde le pas fixe déterministe.
- `core/pixel_canvas` : coquille raylib au-dessus de `viewport`, rend le monde en 320×180 puis l'agrandit.
- Overlay de debug ImGui sur F1, en résolution native (hors canvas).
- Style `.clang-format` (Allman, 2 espaces, 120 colonnes) appliqué par un hook `pre-commit`.
- CI GitHub Actions : job clang-format, puis build + `ctest` sur Linux (gcc/ninja) et Windows (msvc).
- Validé localement : MSYS2 UCRT64 g++ 15.2 + CMake 4.2.3, zéro warning en `-Wall -Wextra -Wpedantic`. CI verte sur cinq jobs (format, couverture, Linux gcc, Windows MSVC, Windows UCRT64).

Points ouverts laissés par cette étape :
- `application`, `pixel_canvas` et les écrans concrets restent non testés : ils sont, par construction, la part GUI irréductible.

## Reste à faire
- **Visée** : `resolve_aim` distingue déjà souris (position absolue) et stick droit (direction), mais aucun écran ne s'en sert encore.
- **Remapping** : `action_map` accepte `bind`/`unbind` à chaud ; reste à persister une table modifiée dans la sauvegarde des options.
- **Resource Manager** + atlas + hot-reload.
- **ECS du donjon** : composants, systèmes, spatial hash.
- **Patterns de tir** : schéma de données puis chargement JSON/TOML.
- Brainstorm gameplay détaillé (à fournir par le joueur).
