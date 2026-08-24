// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/vec2.hpp"

namespace arpg
{

/// A rectangle in window coordinates.
struct viewport_rect
{
  float x = 0.0f;      ///< Left edge, in window pixels.
  float y = 0.0f;      ///< Top edge, in window pixels.
  float width = 0.0f;  ///< In window pixels, so canvas width times the scale.
  float height = 0.0f; ///< In window pixels, so canvas height times the scale.
};

/// Largest whole number by which the canvas fits in the window.
///
/// Whole numbers only, so that a canvas pixel stays a square block of screen
/// pixels. Clamped to 1, so a window smaller than the canvas still renders,
/// cropped rather than blank.
int integer_scale(int canvas_width, int canvas_height, int window_width, int window_height);

/// Window area covered by the upscaled canvas, centered, the space left over
/// being the letterbox.
viewport_rect canvas_destination(int canvas_width, int canvas_height, int window_width, int window_height);

/// Window position expressed in canvas pixels, for aiming with a mouse.
///
/// @return coordinates outside the canvas when the position lies in the
/// letterbox, which callers should expect rather than assume clamped.
vec2 window_to_canvas(vec2 window_position, int canvas_width, int canvas_height, int window_width, int window_height);

} // namespace arpg
