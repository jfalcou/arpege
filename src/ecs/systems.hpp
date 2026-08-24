// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/viewport.hpp>
#include <ecs/components.hpp>
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

/// Refills @p hash with every entity that can be hit.
///
/// Called once per step, before resolving hits, since positions have just
/// moved and last step's cells no longer mean anything.
void rebuild_spatial_hash(const entt::registry& world, spatial_hash& hash);

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
