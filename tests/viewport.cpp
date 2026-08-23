#define TTS_MAIN
#include <tts/tts.hpp>

#include "core/viewport.hpp"

TTS_CASE("Integer scale is the largest factor fitting the window")
{
  TTS_EQUAL(arpg::integer_scale(320, 180, 1280, 720), 4);
  TTS_EQUAL(arpg::integer_scale(320, 180, 1920, 1080), 6);

  // Limited by the tightest axis, here the height.
  TTS_EQUAL(arpg::integer_scale(320, 180, 1920, 720), 4);
};

TTS_CASE("Integer scale never drops below one")
{
  TTS_EQUAL(arpg::integer_scale(320, 180, 160, 90), 1);
  TTS_EQUAL(arpg::integer_scale(320, 180, 0, 0), 1);
};

TTS_CASE("Integer scale tolerates a degenerate canvas")
{
  TTS_EQUAL(arpg::integer_scale(0, 0, 1280, 720), 1);
  TTS_EQUAL(arpg::integer_scale(-320, -180, 1280, 720), 1);
};

TTS_CASE("An exactly fitting window leaves no letterbox")
{
  const auto rect = arpg::canvas_destination(320, 180, 1280, 720);

  TTS_EQUAL(rect.x, 0.0f);
  TTS_EQUAL(rect.y, 0.0f);
  TTS_EQUAL(rect.width, 1280.0f);
  TTS_EQUAL(rect.height, 720.0f);
};

TTS_CASE("The canvas is centered inside a larger window")
{
  // 320x180 upscaled by 4 gives 1280x720 inside a 1400x800 window.
  const auto rect = arpg::canvas_destination(320, 180, 1400, 800);

  TTS_EQUAL(rect.width, 1280.0f);
  TTS_EQUAL(rect.height, 720.0f);
  TTS_EQUAL(rect.x, 60.0f);
  TTS_EQUAL(rect.y, 40.0f);
};

TTS_CASE("Window positions map back to canvas pixels")
{
  const auto origin = arpg::window_to_canvas(arpg::viewport_point{0.0f, 0.0f}, 320, 180, 1280, 720);
  TTS_EQUAL(origin.x, 0.0f);
  TTS_EQUAL(origin.y, 0.0f);

  const auto middle = arpg::window_to_canvas(arpg::viewport_point{640.0f, 360.0f}, 320, 180, 1280, 720);
  TTS_EQUAL(middle.x, 160.0f);
  TTS_EQUAL(middle.y, 90.0f);
};

TTS_CASE("A letterboxed position maps outside the canvas")
{
  // 1400x800 window: the canvas starts at x = 60, so x = 10 is in the border.
  const auto point = arpg::window_to_canvas(arpg::viewport_point{10.0f, 40.0f}, 320, 180, 1400, 800);

  TTS_EXPECT(point.x < 0.0f);
  TTS_EQUAL(point.y, 0.0f);
};

TTS_CASE("Mapping is the inverse of the destination rectangle")
{
  const auto rect = arpg::canvas_destination(320, 180, 1400, 800);
  const auto corner = arpg::window_to_canvas(arpg::viewport_point{rect.x, rect.y}, 320, 180, 1400, 800);

  TTS_EQUAL(corner.x, 0.0f);
  TTS_EQUAL(corner.y, 0.0f);
};
