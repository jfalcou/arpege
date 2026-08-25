// SPDX-License-Identifier: BSL-1.0

#include <ecs/firing_pattern.hpp>

#include <cmath>
#include <numbers>

namespace arpg
{

namespace
{

constexpr float radians_per_degree = std::numbers::pi_v<float> / 180.0f;

/// A full turn shares the arc between every bullet, since its two ends are the
/// same heading and one of them would otherwise carry two bullets.
bool closes_on_itself(float arc)
{
  return std::abs(arc) >= 360.0f;
}

} // namespace

void volley_headings(const firing_pattern& pattern, vec2 towards, int volley, std::vector<vec2>& out)
{
  out.clear();

  if (pattern.bullets <= 0)
  {
    return;
  }

  float base = pattern.spin * static_cast<float>(volley) * radians_per_degree;

  if (pattern.aim == aim_mode::aimed)
  {
    // A target standing exactly on the shooter leaves no direction to fire
    // along, and a heading of nothing would sit on the muzzle.
    if (length_squared(towards) <= 0.0f)
    {
      return;
    }

    base += std::atan2(towards.y, towards.x);
  }

  const float arc = pattern.arc * radians_per_degree;
  const int count = pattern.bullets;

  // A single bullet has nothing to spread over, and dividing by count - 1
  // would ask for a step between one thing and itself.
  const float step = (count < 2) ? 0.0f
                                 : (closes_on_itself(pattern.arc) ? arc / static_cast<float>(count)
                                                                  : arc / static_cast<float>(count - 1));

  // Centred on the heading, so widening the arc opens the volley around where
  // it pointed rather than sweeping it to one side.
  const float first = base - step * static_cast<float>(count - 1) * 0.5f;

  out.reserve(static_cast<std::size_t>(count));

  for (int index = 0; index < count; ++index)
  {
    const float angle = first + step * static_cast<float>(index);
    out.push_back(vec2{std::cos(angle), std::sin(angle)});
  }
}

} // namespace arpg
