// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/vec2.hpp"

namespace arpg
{

// Geometry of the low resolution canvas inside the window. Kept free of any
// windowing or rendering call so it can be exercised without a GPU context.

struct viewport_rect
{
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};

// Largest integer factor by which the canvas fits in the window. Clamped to 1
// so a window smaller than the canvas still renders, cropped.
int integer_scale(int canvas_width, int canvas_height, int window_width, int window_height);

// Window area covered by the upscaled canvas, centered, the remaining space
// being the letterbox.
viewport_rect canvas_destination(int canvas_width, int canvas_height, int window_width, int window_height);

// Window position expressed in canvas pixels. Coordinates fall outside
// [0, canvas_width[ x [0, canvas_height[ when the position is in the letterbox.
vec2 window_to_canvas(vec2 window_position, int canvas_width, int canvas_height, int window_width, int window_height);

} // namespace arpg
