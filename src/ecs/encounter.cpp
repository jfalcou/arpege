// SPDX-License-Identifier: BSL-1.0

#include <ecs/encounter.hpp>

#include <ecs/enemy.hpp>

namespace arpg
{

std::size_t enemies_alive(const entt::registry& world)
{
  return world.view<const enemy_archetype>().size();
}

bool advance_encounter(encounter& fight, const entt::registry& world)
{
  if (fight.state == encounter_state::cleared || enemies_alive(world) != 0)
  {
    return false;
  }

  fight.state = encounter_state::cleared;
  return true;
}

bool enter_portal(exit_portal& way, vec2 player)
{
  const bool inside = length_squared(player - way.centre) < way.radius * way.radius;

  if (!inside)
  {
    way.armed = true;
    return false;
  }

  if (!way.armed)
  {
    return false;
  }

  way.armed = false;
  return true;
}

} // namespace arpg
