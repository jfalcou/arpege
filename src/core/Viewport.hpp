#pragma once

namespace arpg
{

// Geometry of the low resolution canvas inside the window. Kept free of any
// windowing or rendering call so it can be exercised without a GPU context.

struct ViewportRect
{
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};

struct ViewportPoint
{
  float x = 0.0f;
  float y = 0.0f;
};

// Largest integer factor by which the canvas fits in the window. Clamped to 1
// so a window smaller than the canvas still renders, cropped.
int integerScale(int canvasWidth, int canvasHeight, int windowWidth, int windowHeight);

// Window area covered by the upscaled canvas, centered, the remaining space
// being the letterbox.
ViewportRect canvasDestination(int canvasWidth, int canvasHeight, int windowWidth, int windowHeight);

// Window position expressed in canvas pixels. Coordinates fall outside
// [0, canvasWidth[ x [0, canvasHeight[ when the position is in the letterbox.
ViewportPoint windowToCanvas(ViewportPoint windowPosition, int canvasWidth, int canvasHeight, int windowWidth,
                             int windowHeight);

} // namespace arpg
