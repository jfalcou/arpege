// SPDX-License-Identifier: BSL-1.0

#include "core/action_state.hpp"

namespace arpg
{

action_state::action_state()
{
  // Zero would read as "just pressed" and let every action be consumed on the
  // first step.
  m_since_press.fill(never);
}

vec2 movement_direction(const action_set& held, vec2 stick, direction_actions mapping)
{
  if (length_squared(stick) > 0.0f)
  {
    return stick;
  }

  vec2 direction;
  direction.x -= held.test(index_of(mapping.left)) ? 1.0f : 0.0f;
  direction.x += held.test(index_of(mapping.right)) ? 1.0f : 0.0f;
  direction.y -= held.test(index_of(mapping.up)) ? 1.0f : 0.0f;
  direction.y += held.test(index_of(mapping.down)) ? 1.0f : 0.0f;

  // Normalised so a diagonal is not faster than an axis.
  return normalized(direction);
}

aim_input resolve_aim(const input_snapshot& snapshot, input_device active)
{
  if (active == input_device::gamepad)
  {
    return aim_input{false, normalized(snapshot.right_stick)};
  }

  return aim_input{true, snapshot.mouse_position};
}

input_device active_device(const input_snapshot& snapshot, input_device previous)
{
  const bool stick_moved = length_squared(snapshot.left_stick) > 0.0f || length_squared(snapshot.right_stick) > 0.0f;

  if (stick_moved)
  {
    return input_device::gamepad;
  }

  // A held control keeps claiming the device, so the prompts do not flicker
  // while a key stays down.
  for (const binding& control : snapshot.down)
  {
    if (control.device == input_device::gamepad && !snapshot.gamepad_present)
    {
      continue;
    }
    return control.device;
  }

  if (snapshot.mouse_moved)
  {
    return input_device::mouse;
  }

  return previous;
}

void action_state::advance(const action_set& held)
{
  const action_set previous = m_held;

  m_held = held;

  // The first step has no previous frame: treat what is already down as newly
  // pressed rather than as having always been held.
  m_pressed = m_started ? (held & ~previous) : held;
  m_released = m_started ? (previous & ~held) : action_set{};
  m_started = true;

  for (std::size_t i = 0; i < action_count; ++i)
  {
    if (m_pressed.test(i))
    {
      m_since_press[i] = 0;
    }
    else if (m_since_press[i] < never)
    {
      ++m_since_press[i];
    }
  }
}

bool action_state::consume(action target, int window_steps)
{
  const std::size_t i = index_of(target);

  if (m_since_press[i] > window_steps)
  {
    return false;
  }

  m_since_press[i] = never;
  return true;
}

void action_state::flush()
{
  m_since_press.fill(never);
  m_pressed.reset();
  m_released.reset();
}

} // namespace arpg
