// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/vec2.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <cstdint>

namespace arpg
{

/// Where a room stands. What a cleared room owes the player, loot and a choice
/// of way out, hangs off the moment this changes rather than off the count
/// itself.
enum class encounter_state : std::uint8_t
{
  fighting,
  cleared
};

struct encounter
{
  encounter_state state = encounter_state::fighting;

  /// How many the room opened with, which the count left is measured against.
  std::size_t opened_with = 0;
};

/// How many enemies are still standing.
///
/// An archetype is what makes an entity an enemy and a corpse is destroyed
/// outright, so counting archetypes counts the living. No separate tally to
/// keep in step.
std::size_t enemies_alive(const entt::registry& world);

/// Advances @p fight and reports whether the room was cleared on this call.
///
/// True at most once per encounter, so the caller can raise its event without
/// having to remember whether it already did. Enemies arriving afterwards do
/// not reopen a room that has been cleared.
bool advance_encounter(encounter& fight, const entt::registry& world);

/// The way out, which only exists once the room is cleared.
struct exit_portal
{
  vec2 centre{};
  float radius = 12.0f;

  /// Standing where the portal opens must not count as leaving through it: it
  /// only takes the player once they have been outside it at least once.
  bool armed = false;
};

/// True on the step the player enters @p way, false while they stay in it.
bool enter_portal(exit_portal& way, vec2 player);

} // namespace arpg
