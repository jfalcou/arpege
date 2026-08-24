// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/rng.hpp>
#include <core/vec2.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace arpg
{

/// What an enemy is doing.
///
/// A flat enum driving a switch, rather than a hierarchy behind virtual calls:
/// every enemy shares one contiguous component and the processor walks it
/// without chasing pointers.
enum class enemy_state : std::uint8_t
{
  idle,   ///< Has not noticed the player.
  chase,  ///< Closing in.
  attack, ///< Close enough, holding position.
  count   ///< Number of states, not a state.
};

/// The thinking half of an enemy.
struct enemy_brain
{
  enemy_state state = enemy_state::idle;

  /// How long the current state has lasted, in seconds.
  float state_timer = 0.0f;

  /// Which group of steps this enemy reconsiders on.
  ///
  /// Deciding is spread across steps rather than done by everyone at once: a
  /// quarter of the crowd thinks on any given step and the rest keep their
  /// velocity, which divides the cost by four and looks identical.
  std::uint8_t slice = 0;
};

/// How an archetype fights, which decides how close it wants to be.
enum class attack_style : std::uint8_t
{
  /// Hurts by touching, so it closes all the way in.
  melee,

  /// Shoots from a distance and holds there. Closing in would put it in reach
  /// of a weapon it does not have.
  ranged
};

/// A kind of enemy, and what one costs to field.
struct enemy_archetype
{
  /// Points it takes out of the budget of a room.
  int cost = 10;

  int health = 5;
  float speed = 30.0f;
  float radius = 6.0f;

  /// What running into the player costs them.
  int touch = 1;

  /// Distance at which it notices the player.
  float sight = 90.0f;

  /// Distance at which it stops closing in. Short for a body that hurts on
  /// contact, long for something that shoots.
  float reach = 24.0f;

  attack_style style = attack_style::melee;

  /// Seconds between shots. Ignored by a melee archetype.
  float fire_interval = 1.2f;

  float shot_speed = 70.0f;
  float shot_radius = 2.0f;
  int shot_damage = 1;
};

/// Points a room is worth spending on enemies.
///
/// Larger rooms and deeper strates get more, so a room laid out by the
/// generator is dangerous in proportion to its size rather than by luck.
///
/// @param area room area in canvas pixels
/// @param depth how deep the strate is, counted from one
int combat_budget(float area, int depth);

/// Picks which archetypes to field for @p budget.
///
/// Spends greedily on what fits, so a rich room can afford the expensive
/// archetypes while a corridor ends up with a handful of cheap ones. Draws from
/// @p generator, so a seed replays the same room.
///
/// @return indices into @p catalogue, one per enemy to spawn.
std::vector<std::size_t> compose_wave(int budget, std::span<const enemy_archetype> catalogue, rng& generator);

} // namespace arpg
