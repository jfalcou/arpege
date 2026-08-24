// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/vec2.hpp>
#include <core/viewport.hpp>

namespace arpg
{

/// Where the view is centred, in room coordinates.
struct camera_focus
{
  vec2 centre{};
};

/// Moves the view towards @p target and holds it over the room.
///
/// The view never shows anything outside @p room: the centre is held in by half
/// a view, so the edge of the screen lands on the edge of the room rather than
/// on the void beyond it. A room smaller than the view is centred instead,
/// since holding it in would put the far limit before the near one.
///
/// Following is eased rather than snapped, which keeps the picture from
/// juddering on every step of a fixed timestep. The easing is expressed through
/// an exponential so it behaves the same whatever @p dt is: a plain lerp by a
/// constant factor would follow faster on a machine that steps more often.
///
/// @param current where the view is now
/// @param target what it is following, usually the player
/// @param view size of what is on screen, in room coordinates
/// @param stiffness how eagerly it catches up, in units per second. Larger is
///        tighter; around ten feels attached without being rigid.
vec2 follow_camera(vec2 current, vec2 target, viewport_rect room, vec2 view, float dt, float stiffness);

/// Top left corner of the view, from where it is centred.
///
/// This is what rendering offsets by, and what turns a position on screen back
/// into a position in the room.
vec2 view_origin(vec2 centre, vec2 view);

} // namespace arpg
