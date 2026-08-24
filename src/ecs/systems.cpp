// SPDX-License-Identifier: BSL-1.0

#include <ecs/systems.hpp>

#include <algorithm>
#include <vector>

namespace arpg
{

void integrate_motion(entt::registry& world, float dt)
{
  for (auto [entity, place, speed] : world.view<transform, const velocity>().each())
  {
    place.previous = place.position;
    place.position = place.position + speed.value * dt;
  }
}

void expire_lifetimes(entt::registry& world, float dt)
{
  std::vector<entt::entity> expired;

  for (auto [entity, remaining] : world.view<lifetime>().each())
  {
    remaining.remaining -= dt;

    if (remaining.remaining <= 0.0f)
    {
      expired.push_back(entity);
    }
  }

  // Destroying while iterating a view invalidates it, so the pass collects
  // first and destroys after.
  world.destroy(expired.begin(), expired.end());
}

void despawn_out_of_bounds(entt::registry& world, viewport_rect bounds, float margin)
{
  std::vector<entt::entity> gone;

  const float min_x = bounds.x - margin;
  const float min_y = bounds.y - margin;
  const float max_x = bounds.x + bounds.width + margin;
  const float max_y = bounds.y + bounds.height + margin;

  for (auto [entity, place] : world.view<const transform, const projectile>().each())
  {
    if (place.position.x < min_x || place.position.x > max_x || place.position.y < min_y || place.position.y > max_y)
    {
      gone.push_back(entity);
    }
  }

  world.destroy(gone.begin(), gone.end());
}

void rebuild_spatial_hash(const entt::registry& world, spatial_hash& hash)
{
  hash.clear();

  // Only what can be hit: a collider says it has a shape, a team says whose
  // side it is on, and without both there is nothing to resolve against.
  const auto hittable = world.view<const transform, const collider, const team>();

  for (const entt::entity entity : hittable)
  {
    hash.insert(entity, hittable.get<const transform>(entity).position);
  }
}

int resolve_projectile_hits(entt::registry& world, const spatial_hash& hash, std::vector<entt::entity>& scratch)
{
  std::vector<entt::entity> spent;
  int hits = 0;

  for (auto [shot, place, shape, side, hurt] :
       world.view<const transform, const collider, const team, const damage, const projectile>().each())
  {
    hash.query(place.position, shape.radius + hash.cell_size(), scratch);

    for (const entt::entity target : scratch)
    {
      if (target == shot || !world.all_of<transform, collider, team, health>(target))
      {
        continue;
      }

      if (world.get<team>(target).side == side.side)
      {
        continue;
      }

      const auto& other_place = world.get<transform>(target);
      const float reach = shape.radius + world.get<collider>(target).radius;

      // Squared throughout: a square root per pair, thousands of times per
      // step, buys nothing a comparison cannot do.
      if (length_squared(other_place.position - place.position) > reach * reach)
      {
        continue;
      }

      auto& hurt_target = world.get<health>(target);

      // Already dead this step, waiting to be destroyed: shooting a corpse
      // would waste the projectile and queue the corpse twice.
      if (hurt_target.current <= 0)
      {
        continue;
      }

      hurt_target.current -= hurt.amount;
      ++hits;

      spent.push_back(shot);
      if (hurt_target.current <= 0)
      {
        spent.push_back(target);
      }

      // A projectile is spent on its first hit; anything else would let one
      // bullet mow down a whole column.
      break;
    }
  }

  // A target can be finished off while another projectile is still resolving,
  // so the list is deduplicated rather than trusted.
  std::sort(spent.begin(), spent.end());
  spent.erase(std::unique(spent.begin(), spent.end()), spent.end());

  world.destroy(spent.begin(), spent.end());
  return hits;
}

} // namespace arpg
