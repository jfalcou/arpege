// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/vec2.hpp>

#include <cstdint>

namespace arpg
{

/// Where an entity is, and where it was one simulation step ago.
///
/// Both are kept so rendering can interpolate between them and stay smooth
/// whatever the refresh rate.
struct transform
{
  vec2 position{};
  vec2 previous{};
};

/// How fast an entity moves, in canvas pixels per second.
struct velocity
{
  vec2 value{};
};

/// Collision shape. Circles only: a laser is a string of them, and anything
/// else costs more than it is worth at these numbers.
struct collider
{
  float radius = 1.0f;
};

/// Hit points. An entity without one cannot be killed, only despawned.
struct health
{
  int current = 1;
  int maximum = 1;
};

/// How long an entity has left, in seconds. Bullets carry one so a stray shot
/// cannot live forever.
struct lifetime
{
  float remaining = 0.0f;
};

/// Damage dealt on contact.
struct damage
{
  int amount = 1;
};

/// Who an entity fights for.
///
/// Collisions are only ever tested between opposing sides, which is what keeps
/// bullet against bullet off the table.
enum class faction : std::uint8_t
{
  player,
  enemy
};

/// Side an entity belongs to.
struct team
{
  faction side = faction::enemy;
};

/// Marks the entity the player steers. There is exactly one.
struct player_controlled
{
};

/// Marks a projectile, which dies on impact and when it leaves the play area.
struct projectile
{
};

/// Marks something the play area holds in.
///
/// The arena of a bullet hell is the screen itself, so a fighter is kept inside
/// it rather than allowed to wander off. Projectiles carry no such mark: they
/// are meant to leave, and are dropped once they have.
struct confined
{
};

} // namespace arpg
