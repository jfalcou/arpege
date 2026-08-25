// SPDX-License-Identifier: BSL-1.0

#include <ecs/dash.hpp>

#include <algorithm>

namespace arpg
{

bool dashing(const dash_state& dash)
{
  return dash.remaining > 0.0f;
}

vec2 dash_velocity(const dash_state& dash, const dash_profile& profile)
{
  return dashing(dash) ? dash.heading * profile.speed : vec2{};
}

bool advance_dash(dash_state& dash, const dash_profile& profile, bool wants, vec2 steering, float dt)
{
  dash.remaining = std::max(0.0f, dash.remaining - dt);
  dash.cooldown = std::max(0.0f, dash.cooldown - dt);

  // Remembered even mid-dash, so releasing the keys during one does not leave
  // the next without a direction.
  if (length_squared(steering) > 0.0f)
  {
    dash.facing = normalized(steering);
  }

  if (!wants || dashing(dash) || dash.cooldown > 0.0f)
  {
    return false;
  }

  // A player who has not moved yet has nowhere to go, and a dash along nothing
  // would be a cooldown spent on standing still.
  if (length_squared(dash.facing) <= 0.0f)
  {
    return false;
  }

  dash.heading = dash.facing;
  dash.remaining = profile.duration;

  // Counted from the start rather than from the end, so the figure in the data
  // file keeps its meaning when the duration is changed.
  dash.cooldown = std::max(profile.cooldown, profile.duration);

  return true;
}

} // namespace arpg
