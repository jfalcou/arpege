// SPDX-License-Identifier: BSL-1.0

#include "core/viewport.hpp"

#include <algorithm>

namespace arpg
{

int integer_scale(int canvas_width, int canvas_height, int window_width, int window_height)
{
  if (canvas_width <= 0 || canvas_height <= 0)
  {
    return 1;
  }

  const int by_width = window_width / canvas_width;
  const int by_height = window_height / canvas_height;
  return std::max(1, std::min(by_width, by_height));
}

viewport_rect canvas_destination(int canvas_width, int canvas_height, int window_width, int window_height)
{
  const float factor = static_cast<float>(integer_scale(canvas_width, canvas_height, window_width, window_height));
  const float width = static_cast<float>(canvas_width) * factor;
  const float height = static_cast<float>(canvas_height) * factor;

  return viewport_rect{(static_cast<float>(window_width) - width) * 0.5f,
                       (static_cast<float>(window_height) - height) * 0.5f, width, height};
}

vec2 window_to_canvas(vec2 window_position, int canvas_width, int canvas_height, int window_width, int window_height)
{
  const viewport_rect dest = canvas_destination(canvas_width, canvas_height, window_width, window_height);
  const float factor = static_cast<float>(integer_scale(canvas_width, canvas_height, window_width, window_height));

  return vec2{(window_position.x - dest.x) / factor, (window_position.y - dest.y) / factor};
}

} // namespace arpg
