// SPDX-License-Identifier: BSL-1.0

#include <core/deadzone.hpp>

#include <algorithm>

namespace arpg
{

vec2 apply_radial_deadzone(vec2 stick, float inner, float outer)
{
  const float size = length(stick);

  if (size <= inner || outer <= inner)
  {
    return vec2{};
  }

  const float scaled = std::min((size - inner) / (outer - inner), 1.0f);
  return normalized(stick) * scaled;
}

} // namespace arpg
