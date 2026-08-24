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

## Enemies

A room receives a **combat budget** from its area and its depth, and each
archetype costs points: a swarm parasite is cheap, an armoured brute is not. The
budget composes the waves, so a procedurally generated room stays calibrated
without a hand-written script.

Behaviour is a flat state machine over contiguous data, never a hierarchy of
classes behind virtual calls. Heavy thinking is **spread across frames**: a
quarter of the enemies reconsider on any given step, the rest keep their
velocity. Nothing of it is visible, and the cost drops fourfold.

## Firing patterns

A pattern is composed rather than coded, out of four parameterised pieces:

- an **emitter**: where it comes from, how often, how many per volley;
- an **angular distribution**: aimed, fan, circle, spiral;
- a **bullet behaviour**: speed, acceleration, curve, mid-life change, split;
- a **temporal modulation**: phases and pauses.

Described as data and hot-reloaded, so a boss is tuned while it is running.
Scripting is deliberately left out until data proves insufficient.

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
- The audio direction.
