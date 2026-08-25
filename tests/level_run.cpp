// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <world/level_run.hpp>

#include <algorithm>

namespace
{

/// Three rooms in a row, so what a door leads to is known rather than drawn.
arpg::level_layout row()
{
  arpg::level_layout layout;

  layout.rooms.push_back(arpg::level_room{arpg::viewport_rect{0.0f, 0.0f, 300.0f, 200.0f}, arpg::room_role::start});
  layout.rooms.push_back(arpg::level_room{arpg::viewport_rect{340.0f, 0.0f, 300.0f, 200.0f}, arpg::room_role::fight});
  layout.rooms.push_back(arpg::level_room{arpg::viewport_rect{680.0f, 0.0f, 300.0f, 200.0f}, arpg::room_role::boss});

  layout.links.push_back(arpg::level_link{0, 1});
  layout.links.push_back(arpg::level_link{1, 2});

  layout.start = 0;
  layout.boss = 2;

  return layout;
}

} // namespace

TTS_CASE("A level starts at its entrance, with everything still to do")
{
  const arpg::level_run run = arpg::begin_level(row());

  TTS_EQUAL(run.here, 0U);
  TTS_EQUAL(run.cleared.size(), 3U);
  TTS_EXPECT_NOT(arpg::room_is_clear(run));
  TTS_EXPECT_NOT(arpg::level_finished(run));
};

TTS_CASE("A room that is not clear has no way out")
{
  const arpg::level_run run = arpg::begin_level(row());

  // A fight has to be finished rather than walked out of, or every room
  // becomes optional and the level a corridor.
  TTS_EXPECT(arpg::open_doors(run).empty());
};

TTS_CASE("Clearing a room opens its doors")
{
  arpg::level_run run = arpg::begin_level(row());
  arpg::clear_room(run);

  const std::vector<std::size_t> doors = arpg::open_doors(run);

  TTS_EQUAL(doors.size(), 1U);
  TTS_EQUAL(doors[0], 1U);
};

TTS_CASE("Clearing twice changes nothing")
{
  arpg::level_run run = arpg::begin_level(row());

  arpg::clear_room(run);
  arpg::clear_room(run);

  TTS_EXPECT(arpg::room_is_clear(run));
  TTS_EQUAL(arpg::open_doors(run).size(), 1U);
};

TTS_CASE("Only a room a door leads to can be walked into")
{
  arpg::level_run run = arpg::begin_level(row());
  arpg::clear_room(run);

  // The far room is two doors away, and reaching it in one step would be
  // teleporting rather than walking.
  TTS_EXPECT_NOT(arpg::enter_room(run, 2));
  TTS_EQUAL(run.here, 0U);

  TTS_EXPECT(arpg::enter_room(run, 1));
  TTS_EQUAL(run.here, 1U);
};

TTS_CASE("A room nobody named cannot be walked into")
{
  arpg::level_run run = arpg::begin_level(row());
  arpg::clear_room(run);

  TTS_EXPECT_NOT(arpg::enter_room(run, 99));
  TTS_EQUAL(run.here, 0U);
};

TTS_CASE("A room already done stays done when it is walked back into")
{
  arpg::level_run run = arpg::begin_level(row());

  arpg::clear_room(run);
  arpg::enter_room(run, 1);
  arpg::clear_room(run);
  arpg::enter_room(run, 0);

  // Walking back must not refill a room that was emptied, or a level could be
  // farmed by pacing between two rooms.
  TTS_EXPECT(arpg::room_is_clear(run));
};

TTS_CASE("A level ends when the boss room is cleared, not before")
{
  arpg::level_run run = arpg::begin_level(row());

  arpg::clear_room(run);
  arpg::enter_room(run, 1);
  arpg::clear_room(run);
  TTS_EXPECT_NOT(arpg::level_finished(run));

  arpg::enter_room(run, 2);
  arpg::clear_room(run);
  TTS_EXPECT(arpg::level_finished(run));
};

TTS_CASE("An empty level is finished and leads nowhere")
{
  arpg::level_run run = arpg::begin_level(arpg::level_layout{});

  TTS_EXPECT(arpg::open_doors(run).empty());
  TTS_EXPECT_NOT(arpg::enter_room(run, 0));

  // Nothing to clear, so nothing crashes and nothing is claimed.
  arpg::clear_room(run);
  TTS_EXPECT_NOT(arpg::room_is_clear(run));
};

TTS_CASE("A door sits on the wall facing where it leads")
{
  const arpg::viewport_rect left{0.0f, 0.0f, 300.0f, 200.0f};
  const arpg::viewport_rect right{340.0f, 0.0f, 300.0f, 200.0f};

  const arpg::vec2 out = arpg::door_position(left, right);
  TTS_EQUAL(out.x, 300.0f);

  const arpg::vec2 back = arpg::door_position(right, left);
  TTS_EQUAL(back.x, 340.0f);
};

TTS_CASE("A door above sits on the top wall, not the side")
{
  const arpg::viewport_rect below{0.0f, 0.0f, 300.0f, 200.0f};
  const arpg::viewport_rect above{20.0f, -260.0f, 300.0f, 200.0f};

  const arpg::vec2 up = arpg::door_position(below, above);

  TTS_EQUAL(up.y, 0.0f);
  TTS_EXPECT(up.x > 0.0f && up.x < 300.0f);
};

TTS_CASE("A door is slid along its wall towards where it leads")
{
  const arpg::viewport_rect here{0.0f, 0.0f, 300.0f, 400.0f};
  const arpg::viewport_rect low{340.0f, 300.0f, 300.0f, 100.0f};
  const arpg::viewport_rect high{340.0f, 0.0f, 300.0f, 100.0f};

  // Otherwise every door would sit at the middle of its wall, and crossing a
  // level would mean walking back to the centre of each room first.
  TTS_EXPECT(arpg::door_position(here, low).y > arpg::door_position(here, high).y);
};

TTS_CASE("A door never lands in a corner")
{
  const arpg::viewport_rect here{0.0f, 0.0f, 300.0f, 200.0f};

  // Mostly to the side, so the door goes on the right wall, but far enough
  // down that sliding it towards the other room would push it off the end.
  const arpg::viewport_rect askew{4000.0f, 300.0f, 300.0f, 200.0f};

  const arpg::vec2 door = arpg::door_position(here, askew);

  TTS_EQUAL(door.x, here.x + here.width);

  // Reaching a door wedged where two walls meet would be a fight with the
  // collision rather than a step through.
  TTS_EXPECT(door.y > here.y);
  TTS_EXPECT(door.y < here.y + here.height);
};

TTS_CASE("A room narrower than a doorway still gets a door on its wall")
{
  const arpg::viewport_rect sliver{0.0f, 0.0f, 10.0f, 10.0f};
  const arpg::viewport_rect beside{40.0f, 0.0f, 300.0f, 200.0f};

  const arpg::vec2 door = arpg::door_position(sliver, beside);

  TTS_EQUAL(door.x, 10.0f);
  TTS_EXPECT(door.y >= sliver.y - 20.0f && door.y <= sliver.y + sliver.height + 20.0f);
};
