// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/vec2.hpp>
#include <world/level_layout.hpp>

#include <cstddef>
#include <vector>

namespace arpg

{

/// How far a level has been taken.
///
/// Holds no entity and no world: what a room is made of is rebuilt on entering
/// it, and only what must survive that rebuild lives here.
struct level_run
{
  level_layout layout;

  /// One flag per room, in the order the layout names them.
  std::vector<bool> cleared;

  std::size_t here = 0;
};

/// Starts @p layout at its entrance.
level_run begin_level(level_layout layout);

/// Whether the room the player stands in has nothing left in it.
bool room_is_clear(const level_run& run);

/// Marks the current room as done. Doing it twice changes nothing, so a caller
/// need not remember whether it already did.
void clear_room(level_run& run);

/// Rooms the player may walk into from where they stand.
///
/// Empty until the room is clear: a fight has to be finished rather than
/// walked out of, or every room becomes optional and the level a corridor.
std::vector<std::size_t> open_doors(const level_run& run);

/// Moves into @p room, which must be one the doors lead to. Anything else is
/// refused rather than teleporting the player across the level.
bool enter_room(level_run& run, std::size_t room);

/// Whether the boss room has been cleared, which is what ends a level.
bool level_finished(const level_run& run);

/// Where the door to @p to sits on the wall of @p from.
///
/// On the side facing the other room, and slid along that wall to face it, so
/// a door leads towards where it goes rather than out of a corner.
vec2 door_position(const viewport_rect& from, const viewport_rect& to);

} // namespace arpg
