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

namespace
{

/// How many steps a full round of thinking is spread over.
constexpr std::uint64_t thinking_slices = 4;

} // namespace

void advance_brains(entt::registry& world, float dt, std::uint64_t step, vec2 target)
{
  for (auto [entity, brain, place, speed, kind] :
       world.view<enemy_brain, const transform, velocity, const enemy_archetype>().each())
  {
    // Time passes for everyone, whether or not it is their turn to think.
    brain.state_timer += dt;

    if (step % thinking_slices != brain.slice % thinking_slices)
    {
      continue;
    }

    const vec2 towards = target - place.position;
    const float distance_squared = length_squared(towards);
    const float sight_squared = kind.sight * kind.sight;
    const float reach_squared = kind.reach * kind.reach;

    const enemy_state previous = brain.state;

    // Deciding first and acting after, rather than both at once: an enemy that
    // switches state here must move like its new state on this very step, not
    // keep the velocity of the one it just left for a whole round of thinking.
    switch (brain.state)
    {
    case enemy_state::idle:
      if (distance_squared < sight_squared)
      {
        brain.state = enemy_state::chase;
      }
      break;

    case enemy_state::chase:
      // No way back to idle: a room is a fight, and an enemy that has noticed
      // the player does not forget because the player stepped away.
      if (distance_squared < reach_squared)
      {
        brain.state = enemy_state::attack;
      }
      break;

    case enemy_state::attack:
      // Backing off at half again the reach rather than at its exact edge, or
      // the state would swing between the two every round. Any wider and a
      // shooter would hold still firing shots that fall short.
      if (distance_squared > reach_squared * 2.25f)
      {
        brain.state = enemy_state::chase;
      }
      break;

    case enemy_state::count:
      break;
    }

    speed.value = (brain.state == enemy_state::chase) ? normalized(towards) * kind.speed : vec2{};

    if (brain.state != previous)
    {
      brain.state_timer = 0.0f;
    }
  }
}

void advance_appearances(entt::registry& world, float dt)
{
  for (auto [entity, look] : world.view<appearance>().each())
  {
    look.elapsed += dt;
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

void confine_to_bounds(entt::registry& world, viewport_rect bounds)
{
  for (auto [entity, place, shape] : world.view<transform, const collider, const confined>().each())
  {
    const float left = bounds.x + shape.radius;
    const float top = bounds.y + shape.radius;
    const float right = bounds.x + bounds.width - shape.radius;
    const float bottom = bounds.y + bounds.height - shape.radius;

    // A room narrower than the entity would put the far edge before the near
    // one; centring is the only sensible answer, and it keeps the clamp from
    // depending on which axis is applied first.
    place.position.x = (left <= right) ? std::clamp(place.position.x, left, right) : bounds.x + bounds.width * 0.5f;
    place.position.y = (top <= bottom) ? std::clamp(place.position.y, top, bottom) : bounds.y + bounds.height * 0.5f;
  }
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

entt::entity spawn_projectile(entt::registry& world, const shot_recipe& recipe)
{
  const entt::entity shot = world.create();

  world.emplace<transform>(shot, recipe.from, recipe.from);
  world.emplace<velocity>(shot, recipe.heading * recipe.speed);
  world.emplace<collider>(shot, recipe.radius);
  world.emplace<team>(shot, recipe.side);
  world.emplace<damage>(shot, recipe.hurt);
  world.emplace<lifetime>(shot, recipe.life);
  world.emplace<projectile>(shot);

  return shot;
}

int fire_enemy_weapons(entt::registry& world, float dt, vec2 target, std::vector<vec2>& headings)
{
  int fired = 0;

  for (auto [entity, gun, brain, kind, place, shape] :
       world.view<weapon, const enemy_brain, const enemy_archetype, const transform, const collider>().each())
  {
    gun.cooldown -= dt;

    if (kind.style != attack_style::ranged || brain.state != enemy_state::attack)
    {
      continue;
    }

    if (gun.cooldown > 0.0f || kind.shots.interval <= 0.0f)
    {
      continue;
    }

    volley_headings(kind.shots, target - place.position, gun.volley, headings);

    // An aimed pattern yields nothing when the target stands on the muzzle,
    // which must not consume the volley: the shooter tries again next step
    // rather than losing its turn.
    if (headings.empty())
    {
      continue;
    }

    gun.cooldown = kind.shots.interval;
    ++gun.volley;

    for (const vec2 heading : headings)
    {
      // Started at the edge of the body rather than its centre, or a wide
      // enemy would spawn its own shot inside itself.
      spawn_projectile(world, shot_recipe{.from = place.position + heading * (shape.radius + kind.shots.radius),
                                          .heading = heading,
                                          .speed = kind.shots.speed,
                                          .radius = kind.shots.radius,
                                          .hurt = kind.shots.damage,
                                          .life = kind.shots.life,
                                          .side = faction::enemy});
      ++fired;
    }
  }

  return fired;
}

void tick_invulnerability(entt::registry& world, float dt)
{
  for (auto [entity, shield] : world.view<invulnerable>().each())
  {
    shield.remaining = std::max(0.0f, shield.remaining - dt);
  }
}

int resolve_contact_damage(entt::registry& world, const spatial_hash& hash, std::vector<entt::entity>& scratch)
{
  std::vector<entt::entity> slain;
  int hits = 0;

  // Whatever deals contact damage and is not a projectile: an enemy body,
  // later a hazard on the floor. A projectile is spent on impact and is
  // resolved elsewhere.
  for (auto [toucher, place, shape, side, hurt] :
       world.view<const transform, const collider, const team, const damage>(entt::exclude<projectile>).each())
  {
    hash.query(place.position, shape.radius + hash.cell_size(), scratch);

    for (const entt::entity target : scratch)
    {
      // The grid is a snapshot, so an earlier pass of this step may have
      // destroyed something still filed in it. Said outright rather than left
      // to the registry answering false for a dead entity.
      if (target == toucher || !world.valid(target) || !world.all_of<transform, collider, team, health>(target))
      {
        continue;
      }

      if (world.get<team>(target).side == side.side)
      {
        continue;
      }

      auto* shield = world.try_get<invulnerable>(target);

      if (shield != nullptr && shield->remaining > 0.0f)
      {
        continue;
      }

      auto& hurt_target = world.get<health>(target);

      if (hurt_target.current <= 0)
      {
        continue;
      }

      const float reach = shape.radius + world.get<collider>(target).radius;

      if (length_squared(world.get<transform>(target).position - place.position) > reach * reach)
      {
        continue;
      }

      hurt_target.current -= hurt.amount;
      ++hits;

      if (shield != nullptr)
      {
        shield->remaining = shield->duration;
      }

      if (hurt_target.current <= 0)
      {
        slain.push_back(target);
      }

      // One body can only press against one target at a time, and stopping
      // here keeps a crowd from being mown down by a single walker.
      break;
    }
  }

  std::sort(slain.begin(), slain.end());
  slain.erase(std::unique(slain.begin(), slain.end()), slain.end());

  world.destroy(slain.begin(), slain.end());
  return hits;
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
      // The grid is a snapshot, so an earlier pass of this step may have
      // destroyed something still filed in it. Said outright rather than left
      // to the registry answering false for a dead entity.
      if (target == shot || !world.valid(target) || !world.all_of<transform, collider, team, health>(target))
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
