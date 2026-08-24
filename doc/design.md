<!-- SPDX-License-Identifier: BSL-1.0 -->

# Design

What the game is, and the decisions the code answers to.

Proper nouns are written in French because that is the language they were coined
in, and they are the canonical name of a thing rather than its final wording.
The game is meant to be localised, so every one of them is translatable, and no
string the player reads should ever be written into the code. See
[Localisation](#localisation).

## The pitch

A Bureau sends disposable employees to die in the Failles, and files the
paperwork afterwards. The horror is cosmic; the framing is administrative, and
the coldness of the second sharpens the first rather than defusing it.

A run goes down through **strates**, each a themed layer with a name of its own:
le Lazaret de Porcelaine, la Cathédrale Inversée, l'Abattoir Géométrique, le
Récif de Suif, la Mer de Céruse — the Porcelain Lazaret, the Inverted Cathedral,
the Geometric Slaughterhouse, the Tallow Reef, the Ceruse Sea.

## The loop

1. **The Bureau.** Between runs. Read memos, spend gold, sign a contract with a
   mercenary from the board.
2. **The strate map.** Pick a route between nodes. Branches force a choice
   between two biomes rather than a single corridor.
3. **The rooms.** Bullet hell. A room is cleared when its combat budget is spent
   and nothing is left standing.
4. **The exit.** Every strate ends on two portals, and picking one is the tension
   of the run:
   - the **white portal**, stable, moves on or returns to the Bureau;
   - the **black portal with red wisps**, unstable, a shortcut into a corrupted
     biome, worth more and likely to kill.

## Mercenaries

The player does not choose a class, they hire someone. The board of the Bureau
pins candidate files: a rarity (junior, confirmé, senior), a price in gold, and
three abilities — a passive, an attack, a dodge.

Each profile is stored in **two halves that never mix**:

- `narrative`, what the player reads: the name of the ability and its prose;
- `engine`, what the game runs: identifiers such as `psv_penance_armor`,
  `atk_inquisitor_cleaver`, `dod_iron_charge`, plus the raw numbers.

A file that only holds prose cannot be played, and a file that only holds numbers
cannot be read. Keeping both apart is what lets the writing and the systems move
independently.

Unlocks stay **horizontal**: new options, weapons and profiles, never stacking
percentages.

## Combat

Bullets are the enemy, not the enemies. What that implies:

- **Circles only** for collisions; a laser is a string of circles.
- The player hitbox is a few pixels, far smaller than the sprite, and shown in
  focus mode.
- Pairs that are tested: enemy bullets against the player, player shots against
  enemies, the player against enemies and pickups. **Never bullet against
  bullet.**
- Squared distances throughout. A square root per pair, thousands of times per
  frame, buys nothing.
- Bullets off screen, plus a margin, die immediately.

## The camera

A room is **larger than the screen** and the camera follows the player across
it. Rooms are places to move through rather than single tableaux, which is what
a run made of connected rooms and corridors asks for.

That is a deliberate trade. A bullet hell in the strict sense keeps everything
on one fixed screen, because a projectile arriving from outside the view cannot
be dodged and punishes without warning. Following the player buys room to move
and an exploration that a fixed screen cannot give, and it owes the player
something in return:

- **spawns happen on screen**, or announced, never silently behind the edge;
- **off-screen threats are signalled** at the rim rather than left to be
  discovered by taking the hit;
- the denser the pattern, the closer the encounter should be framed.

The view never shows past the walls: the centre is held half a view from each
edge, and a room smaller than the view is centred instead. Following is eased
through an exponential so it behaves identically whatever the step rate, where a
plain lerp would trail differently on a machine that steps more often.

## Enemies

A room receives a **combat budget** from its area and its depth, and each
archetype costs points: a swarm parasite is cheap, an armoured brute is not. The
budget composes the waves, so a procedurally generated room stays calibrated
without a hand-written script.

An archetype either **closes in and hurts by touching**, or **holds at a
distance and shoots**. That single choice decides how close it wants to be, so a
shooter that walked all the way in would end up in reach of a weapon it does not
have, and a body that stopped short would never reach anyone. The shooters are
what make a room a bullet hell rather than a crowd to outrun.

Behaviour is a flat state machine over contiguous data, never a hierarchy of
classes behind virtual calls. Heavy thinking is **spread across frames**: a
quarter of the enemies reconsider on any given step, the rest keep their
velocity. Nothing of it is visible, and the cost drops fourfold.

## Data files

Content is described in **Lua**, read through sol2 and loaded by a single
script host. Lua rather than JSON or TOML: the files that state figures today
are the ones that will describe boss phases later, and a format that can only
hold literals would have to be replaced at that point rather than extended. A
roster already gains from it — a family of related enemies is written once and
varied in a loop instead of copied three times.

The host opens **base, math, string and table, and nothing else**. A data file
has no business opening files, reading the clock or loading libraries of its
own: an edit meant to tune a number must not be able to reach outside the game.

Loading **validates and refuses**. An archetype that costs nothing would be
bought forever by the wave budget; one that wakes closer than the player can
strike would die without ever noticing. A file with one bad entry loads none of
it, because half a roster fields a wave nobody designed, and that shows up as a
strange fight rather than as a message.

Files are watched and **re-read while the game runs**, once a second rather
than every frame: a file changes when a human saves it. A read that fails
keeps the last version that made sense and shows the reason on screen, since a
file caught halfway through an edit would otherwise empty the room. What is
re-read re-forms the room **from the same seed**, so a changed figure is judged
against the room it was changed for.

The build mirrors `assets/` next to the executable, and that copy is
overwritten by the next build. Setting `ARPG_ASSETS` points the game at the
working copy instead, so the file being tuned live is the one under version
control.

One roster file holds every archetype while there are a handful of them. Past
a dozen it becomes one file per enemy under a scanned directory, which changes
neither the loading API nor its tests: the split is deferred because deferring
it costs nothing, not because it is undecided.

Scripted behaviour, when an enemy eventually needs its own, runs **at events
and not at every step**: on spawn, on a phase change, on death. A script called
once per enemy per simulation step would be fifteen trips into an interpreter
sixty times a second, and would pull the hot loop out of the contiguous data
the flat state machine exists to keep it in. Lua describes what a behaviour is
composed of; the C++ systems unroll it. A boss with three phases costs three
calls over a fight. Whatever carries such a behaviour reaches the ECS as a
handle into a table held by the data layer, since an archetype is copied into
every entity that uses it and must stay trivially copyable.

## Firing patterns

A pattern is composed rather than coded, out of four parameterised pieces:

- an **emitter**: where it comes from, how often, how many per volley;
- an **angular distribution**: aimed, fan, circle, spiral;
- a **bullet behaviour**: speed, acceleration, curve, mid-life change, split;
- a **temporal modulation**: phases and pauses.

Described as data and hot-reloaded, so a boss is tuned while it is running.
Since that data is Lua, a pattern that outgrows a table of parameters becomes a
function without anyone having to introduce a second format for it.

## Determinism

A run carries a **seed**, and the same seed must give the same run: it is what
makes daily runs possible, lets a player share a route, and turns a bug report
into something reproducible.

That rules out the distributions of the standard library. The engines there are
specified, but `std::uniform_int_distribution` is not: identical seeds produce
different numbers on different implementations, so a shared seed would name a
different level depending on who built the game. The generator and its bounded
draw are therefore written out, and a test pins the exact stream so a change to
the algorithm cannot slip through unnoticed.

Two sources of randomness, kept apart:

- **content**, drawn from the seeded generator: layout, wave composition, spawn
  positions, loot. It must replay.
- **cosmetic**, free to use anything: particles, screen shake, pitch variation.
  Nobody can tell whether two runs shook the screen the same way.

## Localisation

The game is localised at the end, which is a decision about the code from the
start: a string shown to the player is a **key resolved at display time**, never
a literal sitting in a call to the renderer. Retrofitting that once the text has
spread through the screens costs far more than carrying it from the beginning.

Two consequences worth stating plainly:

- Proper nouns are content, not identifiers. `strate_porcelain_lazaret` names the
  thing; what appears on screen is whatever the current language says it is.
- The identifiers of the engine half of a mercenary file are *not* text and are
  never translated: `atk_inquisitor_cleaver` is a symbol the code matches on,
  and it stays in English like the rest of the code.

The current screens still hold their strings inline. That is fine while there
are three of them, and it is the first thing to fix before there are thirty.

## What is deliberately not decided

- The names and roster of the mercenaries beyond the handful already written.
- Whether the strate map is a graph the player walks or a list of nodes.
- How an off-screen threat is signalled, which the camera decision now owes.
- The audio direction.
