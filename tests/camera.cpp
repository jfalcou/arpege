// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <core/camera.hpp>

namespace
{

constexpr arpg::viewport_rect room{0.0f, 0.0f, 640.0f, 360.0f};
constexpr arpg::vec2 view{320.0f, 180.0f};

} // namespace

TTS_CASE("The view closes in on what it follows")
{
  arpg::vec2 centre{320.0f, 180.0f};
  const arpg::vec2 target{400.0f, 180.0f};

  const float before = target.x - centre.x;
  centre = arpg::follow_camera(centre, target, room, view, 1.0f / 60.0f, 10.0f);
  const float after = target.x - centre.x;

  TTS_EXPECT(after < before);
  TTS_EXPECT(after > 0.0f);
};

TTS_CASE("Following long enough arrives")
{
  arpg::vec2 centre{320.0f, 180.0f};
  const arpg::vec2 target{400.0f, 200.0f};

  for (int i = 0; i < 600; ++i)
  {
    centre = arpg::follow_camera(centre, target, room, view, 1.0f / 60.0f, 10.0f);
  }

  TTS_ULP_EQUAL(centre.x, target.x, 64.0);
  TTS_ULP_EQUAL(centre.y, target.y, 64.0);
};

TTS_CASE("Easing does not depend on how often it is stepped")
{
  const arpg::vec2 start{320.0f, 180.0f};
  const arpg::vec2 target{600.0f, 180.0f};

  arpg::vec2 coarse = start;
  coarse = arpg::follow_camera(coarse, target, room, view, 1.0f / 30.0f, 10.0f);

  arpg::vec2 fine = start;
  fine = arpg::follow_camera(fine, target, room, view, 1.0f / 60.0f, 10.0f);
  fine = arpg::follow_camera(fine, target, room, view, 1.0f / 60.0f, 10.0f);

  // Two steps of a sixtieth must land where one step of a thirtieth does, or
  // the camera would trail differently on a machine that steps more often.
  TTS_ULP_EQUAL(coarse.x, fine.x, 512.0);
};

TTS_CASE("The view never shows past the edge of the room")
{
  // Following something pinned in a corner: the centre must stop half a view
  // away from the walls rather than reveal the void outside.
  arpg::vec2 centre{320.0f, 180.0f};

  for (int i = 0; i < 600; ++i)
  {
    centre = arpg::follow_camera(centre, arpg::vec2{0.0f, 0.0f}, room, view, 1.0f / 60.0f, 10.0f);
  }

  TTS_EQUAL(centre.x, 160.0f);
  TTS_EQUAL(centre.y, 90.0f);
};

TTS_CASE("The far edge holds the view in as well")
{
  arpg::vec2 centre{320.0f, 180.0f};

  for (int i = 0; i < 600; ++i)
  {
    centre = arpg::follow_camera(centre, arpg::vec2{5000.0f, 5000.0f}, room, view, 1.0f / 60.0f, 10.0f);
  }

  TTS_EQUAL(centre.x, 480.0f);
  TTS_EQUAL(centre.y, 270.0f);
};

TTS_CASE("A room smaller than the view is centred")
{
  // Holding the view in would put the far limit before the near one, which is
  // not something to clamp against.
  constexpr arpg::viewport_rect cramped{0.0f, 0.0f, 100.0f, 80.0f};

  const arpg::vec2 centre = arpg::follow_camera(arpg::vec2{}, arpg::vec2{500.0f, 500.0f}, cramped, view, 1.0f, 10.0f);

  TTS_EQUAL(centre.x, 50.0f);
  TTS_EQUAL(centre.y, 40.0f);
};

TTS_CASE("A room offset from the origin is handled")
{
  constexpr arpg::viewport_rect offset{1000.0f, 500.0f, 640.0f, 360.0f};

  const arpg::vec2 centre =
      arpg::follow_camera(arpg::vec2{1320.0f, 680.0f}, arpg::vec2{0.0f, 0.0f}, offset, view, 1.0f, 100.0f);

  TTS_EQUAL(centre.x, 1160.0f);
  TTS_EQUAL(centre.y, 590.0f);
};

TTS_CASE("No stiffness snaps straight to the target")
{
  const arpg::vec2 centre = arpg::follow_camera(arpg::vec2{320.0f, 180.0f}, arpg::vec2{400.0f, 200.0f}, room, view,
                                                1.0f / 60.0f, 0.0f);

  TTS_EQUAL(centre.x, 400.0f);
  TTS_EQUAL(centre.y, 200.0f);
};

TTS_CASE("The origin of the view is half a view from its centre")
{
  const arpg::vec2 origin = arpg::view_origin(arpg::vec2{320.0f, 180.0f}, view);

  TTS_EQUAL(origin.x, 160.0f);
  TTS_EQUAL(origin.y, 90.0f);
};

TTS_CASE("A room grown by a margin keeps its middle")
{
  const arpg::viewport_rect room{100.0f, 50.0f, 400.0f, 300.0f};
  const arpg::viewport_rect grown = arpg::with_margin(room, 16.0f);

  TTS_EQUAL(grown.x, 84.0f);
  TTS_EQUAL(grown.y, 34.0f);
  TTS_EQUAL(grown.width, 432.0f);
  TTS_EQUAL(grown.height, 332.0f);

  // Grown on every side, so what the view is held inside opens around the room
  // rather than sliding off it.
  TTS_EQUAL(grown.x + grown.width * 0.5f, room.x + room.width * 0.5f);
  TTS_EQUAL(grown.y + grown.height * 0.5f, room.y + room.height * 0.5f);
};

TTS_CASE("A margin lets the view show what stands past a wall")
{
  const arpg::viewport_rect room{0.0f, 0.0f, 640.0f, 360.0f};
  const arpg::vec2 view{320.0f, 180.0f};

  // A body against the top wall, held there by a collider of two pixels while
  // its picture stands sixteen tall.
  const arpg::vec2 against_the_wall{320.0f, 2.0f};

  const arpg::vec2 tight = arpg::follow_camera(against_the_wall, against_the_wall, room, view, 0.0f, 0.0f);
  const arpg::vec2 loose =
      arpg::follow_camera(against_the_wall, against_the_wall, arpg::with_margin(room, 16.0f), view, 0.0f, 0.0f);

  // Without the margin the view stops at the wall and cuts the picture in two.
  TTS_EXPECT(arpg::view_origin(loose, view).y < arpg::view_origin(tight, view).y);
};
