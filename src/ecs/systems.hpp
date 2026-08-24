// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/viewport.hpp>
#include <ecs/components.hpp>
#include <ecs/enemy.hpp>
#include <ecs/spatial_hash.hpp>

#include <entt/entity/registry.hpp>

#include <vector>

namespace arpg
{

/// Advances every moving entity by one step, remembering where it was.
///
/// The previous position is what rendering interpolates from, so nothing else
/// is allowed to write it.
void integrate_motion(entt::registry& world, float dt);

/// Ages every lifetime and destroys what ran out.
void expire_lifetimes(entt::registry& world, float dt);

/// Destroys projectiles that left @p bounds by more than @p margin.
///
/// Only projectiles: an enemy walking in from off screen is not a stray shot.
/// The margin leaves room for something to be spawned just outside and fly in.
void despawn_out_of_bounds(entt::registry& world, viewport_rect bounds, float margin);

/// Keeps whatever is marked confined inside @p bounds.
///
/// The whole circle is held in, not its centre, so nothing ends up half buried
/// in a wall. Only the position is corrected: velocity is left alone, so
/// pushing into an edge slides along it instead of sticking.
void confine_to_bounds(entt::registry& world, viewport_rect bounds);

/// Refills @p hash with every entity that can be hit.
///
/// Called once per step, before resolving hits, since positions have just
/// moved and last step's cells no longer mean anything.
void rebuild_spatial_hash(const entt::registry& world, spatial_hash& hash);

/// Advances the enemies: notice the player, close in, hold at reach.
///
/// Only a quarter of them reconsider on any given step, chosen by their slice
/// against @p step. The rest keep the velocity they were given, which is
/// indistinguishable on screen and divides the cost by four.
///
/// @param step a counter increased once per simulation step
/// @param target where the player is
void advance_brains(entt::registry& world, float dt, std::uint64_t step, vec2 target);

/// Counts invulnerability down.
void tick_invulnerability(entt::registry& world, float dt);

/// Applies contact damage between opposing sides.
///
/// Unlike a projectile, whatever deals the damage survives it: an enemy walking
/// into the player keeps walking. A target still inside its invulnerability is
/// passed over, which is what stops contact from draining a life bar at sixty
/// steps a second.
///
/// @param scratch reused between calls so querying does not allocate.
/// @return how many hits landed.
int resolve_contact_damage(entt::registry& world, const spatial_hash& hash, std::vector<entt::entity>& scratch);

/// Applies projectile damage to the opposing side.
///
/// Only opposing sides are tested, which is what keeps bullet against bullet
/// out of the loop entirely. A projectile dies on its first hit; a target dies
/// when its health reaches zero.
///
/// @param scratch reused between calls so querying does not allocate every
///        frame; its contents on entry are irrelevant.
/// @return how many hits were applied, which the caller turns into sounds and
///         particles rather than having this reach for them.
int resolve_projectile_hits(entt::registry& world, const spatial_hash& hash, std::vector<entt::entity>& scratch);

} // namespace arpg
