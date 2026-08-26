// SPDX-License-Identifier: BSL-1.0

#include <core/camera.hpp>

#include <algorithm>
#include <cmath>

namespace arpg
{

namespace
{

/// Holds @p value so that a window of @p extent stays inside [@p low, @p high].
float hold_within(float value, float low, float high, float extent)
{
  const float half = extent * 0.5f;
  const float near_limit = low + half;
  const float far_limit = high - half;

  // A room narrower than the view cannot hold it; centring is the only sensible
  // answer, and it keeps the clamp off a reversed range.
  return (near_limit <= far_limit) ? std::clamp(value, near_limit, far_limit) : (low + high) * 0.5f;
}

} // namespace

vec2 follow_camera(vec2 current, vec2 target, viewport_rect room, vec2 view, float dt, float stiffness)
{
  vec2 moved = target;

  if (stiffness > 0.0f && dt > 0.0f)
  {
    // Framerate independent easing: the share of the remaining distance covered
    // over dt, rather than a fixed share per step.
    const float factor = 1.0f - std::exp(-stiffness * dt);
    moved = current + (target - current) * factor;
  }

  return vec2{hold_within(moved.x, room.x, room.x + room.width, view.x),
              hold_within(moved.y, room.y, room.y + room.height, view.y)};
}

viewport_rect with_margin(viewport_rect room, float margin)
{
  return viewport_rect{room.x - margin, room.y - margin, room.width + 2.0f * margin, room.height + 2.0f * margin};
}

vec2 view_origin(vec2 centre, vec2 view)
{
  return centre - view * 0.5f;
}

} // namespace arpg
