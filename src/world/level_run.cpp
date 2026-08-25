// SPDX-License-Identifier: BSL-1.0

#include <world/level_run.hpp>

#include <algorithm>
#include <cmath>

namespace arpg
{

namespace
{

/// How far inside the wall a door sits, so it is reachable without touching a
/// corner where two walls meet.
constexpr float door_inset = 24.0f;

vec2 centre_of(const viewport_rect& room)
{
  return vec2{room.x + room.width * 0.5f, room.y + room.height * 0.5f};
}

} // namespace

level_run begin_level(level_layout layout)
{
  level_run run;
  run.here = layout.start;
  run.cleared.assign(layout.rooms.size(), false);
  run.layout = std::move(layout);

  return run;
}

bool room_is_clear(const level_run& run)
{
  return run.here < run.cleared.size() && run.cleared[run.here];
}

void clear_room(level_run& run)
{
  if (run.here < run.cleared.size())
  {
    run.cleared[run.here] = true;
  }
}

std::vector<std::size_t> open_doors(const level_run& run)
{
  if (!room_is_clear(run))
  {
    return {};
  }

  return neighbours_of(run.layout, run.here);
}

bool enter_room(level_run& run, std::size_t room)
{
  const std::vector<std::size_t> doors = open_doors(run);

  if (std::find(doors.begin(), doors.end(), room) == doors.end())
  {
    return false;
  }

  run.here = room;
  return true;
}

bool level_finished(const level_run& run)
{
  return run.layout.boss < run.cleared.size() && run.cleared[run.layout.boss];
}

vec2 door_position(const viewport_rect& from, const viewport_rect& to)
{
  const vec2 mine = centre_of(from);
  const vec2 theirs = centre_of(to);

  const float dx = theirs.x - mine.x;
  const float dy = theirs.y - mine.y;

  // The side is decided by whichever way the other room lies furthest, so two
  // rooms side by side never get a door on the floor between them.
  // Clamped between the two ends of the wall, taking whichever bound is the
  // lower: a room narrower than twice the inset would otherwise give a range
  // running backwards, and std::clamp on one of those is undefined.
  if (std::abs(dx) >= std::abs(dy))
  {
    const float low = std::min(from.y + door_inset, from.y + from.height - door_inset);
    const float high = std::max(from.y + door_inset, from.y + from.height - door_inset);

    return vec2{(dx >= 0.0f) ? from.x + from.width : from.x, std::clamp(theirs.y, low, high)};
  }

  const float low = std::min(from.x + door_inset, from.x + from.width - door_inset);
  const float high = std::max(from.x + door_inset, from.x + from.width - door_inset);

  return vec2{std::clamp(theirs.x, low, high), (dy >= 0.0f) ? from.y + from.height : from.y};
}

} // namespace arpg
