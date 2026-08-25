// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/rng.hpp>
#include <core/vec2.hpp>
#include <core/viewport.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace arpg
{

/// What a room is there for.
///
/// The role is what the rest of the game reads: a fight fills with a wave, the
/// boss room holds the portals that end the level, the station is where loot
/// is sent back to the Bureau.
enum class room_role : std::uint8_t
{
  start,
  fight,
  boss,
  station
};

struct level_room
{
  /// In world units, the same ones the camera and the colliders work in.
  viewport_rect bounds{};

  room_role role = room_role::fight;
};

/// A way through, between two rooms named by their index in the layout.
struct level_link
{
  std::size_t from = 0;
  std::size_t to = 0;
};

/// The whole of a level, and the only thing a generator produces.
///
/// Every generator answers with this shape, which is what lets a biome pick
/// between them: were each to return a structure of its own, the choice could
/// not be data, and there would be as many pipelines as there are algorithms.
struct level_layout
{
  std::vector<level_room> rooms;
  std::vector<level_link> links;

  std::size_t start = 0;
  std::size_t boss = 0;
};

/// How the rooms are laid out, which is what a biome chooses.
///
/// The shape decides how it looks; the topology it comes with decides how it
/// plays. Both generators here grow a tree, so a level has no loop yet.
enum class level_shape : std::uint8_t
{
  /// Recursive splitting of one box. Rooms are axis aligned and tightly
  /// packed: the architecture of something built by people, however mad.
  rigid,

  /// Rooms accreted one against another, each attached to a wall of one
  /// already placed. Irregular, but always made of actual rooms, which a
  /// bullet hell needs: an arena has to be readable at a glance.
  organic
};

struct level_recipe
{
  level_shape shape = level_shape::rigid;

  /// How many rooms a level holds, drawn between the two so that two levels
  /// of the same biome are not the same walk. Both generators may fall short
  /// of what is drawn rather than force a room where none fits, so the upper
  /// bound is a ceiling and not a promise.
  int rooms_min = 6;
  int rooms_max = 8;

  /// Per axis, since a view is wider than it is tall: one figure for both
  /// would make a room that is a screen and a half wide four screens deep.
  vec2 room_min{200.0f, 200.0f};
  vec2 room_max{420.0f, 420.0f};

  /// What is left between two rooms, which is where a corridor goes.
  float spacing = 48.0f;

  /// How much of an ordinary room a room holding no fight takes. A station is
  /// a desk and the clerk behind it, not an arena, and giving it the size of
  /// one would promise a fight that never comes.
  float service_scale = 0.5f;
};

/// Lays a level out, drawing every choice from @p generator.
///
/// Deterministic: the same recipe and the same seed give the same level, which
/// is what lets a level be derived from the seed of a posting rather than
/// stored.
level_layout generate_level(const level_recipe& recipe, rng& generator);

/// Whether every room can be reached from the start by following the links.
///
/// A generator that cannot promise this has produced a level someone can be
/// shut out of, so it is worth asking rather than assuming.
bool fully_connected(const level_layout& layout);

/// Rooms reachable in one step from @p room.
std::vector<std::size_t> neighbours_of(const level_layout& layout, std::size_t room);

} // namespace arpg
