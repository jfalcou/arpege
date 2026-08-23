// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/action.hpp"
#include "core/action_map.hpp"
#include "core/input_snapshot.hpp"
#include "core/vec2.hpp"

#include <array>

namespace arpg
{

// Where the player is aiming. The input layer does not know where the player
// entity is, so it reports either a canvas position (mouse) or a direction
// (right stick) and lets the gameplay turn that into a heading.
struct aim_input
{
  bool absolute = false;
  vec2 value{};
};

// Which four actions stand for the four directions. A dungeon and a menu bind
// different ones, so both go through the same code.
struct direction_actions
{
  action up = action::move_up;
  action down = action::move_down;
  action left = action::move_left;
  action right = action::move_right;
};

inline constexpr direction_actions movement_actions{action::move_up, action::move_down, action::move_left,
                                                    action::move_right};

inline constexpr direction_actions menu_actions{action::menu_up, action::menu_down, action::menu_left,
                                                action::menu_right};

// Direction, normalised. The stick wins when it is out of its deadzone,
// otherwise the digital actions give the usual eight directions.
vec2 movement_direction(const action_set& held, vec2 stick, direction_actions mapping = movement_actions);

// Aim from the mouse when it is the device in use, from the right stick
// otherwise. Returns a zero direction when neither says anything.
aim_input resolve_aim(const input_snapshot& snapshot, input_device active);

// Device that last said something, so the UI can show matching prompts. Falls
// back to the previous one while nothing happens.
input_device active_device(const input_snapshot& snapshot, input_device previous);

// Edge detection and input buffering, advanced once per simulation step.
class action_state
{
public:
  // A press stays claimable for this many steps, a tenth of a second at 60 Hz.
  static constexpr int default_buffer_steps = 6;

  action_state();

  void advance(const action_set& held);

  bool held(action target) const { return m_held.test(index_of(target)); }
  bool pressed(action target) const { return m_pressed.test(index_of(target)); }
  bool released(action target) const { return m_released.test(index_of(target)); }

  // Claims a press that happened within the window, including earlier steps.
  // Returns false once claimed, so an action is never consumed twice.
  bool consume(action target, int window_steps = default_buffer_steps);

  // Drops every buffered press, for a screen change or a cutscene.
  void flush();

private:
  static constexpr int never = 1 << 20;

  action_set m_held;
  action_set m_pressed;
  action_set m_released;
  std::array<int, action_count> m_since_press{};
  bool m_started = false;
};

} // namespace arpg
