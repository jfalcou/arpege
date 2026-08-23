// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstddef>

namespace arpg
{

// What the gameplay reacts to. Nothing outside the input layer ever mentions a
// key or a button, so remapping and AZERTY/QWERTY are a matter of bindings.
enum class action
{
  move_up,
  move_down,
  move_left,
  move_right,
  shoot,
  dash,
  focus,
  pause,
  menu_up,
  menu_down,
  menu_left,
  menu_right,
  confirm,
  cancel,
  count
};

inline constexpr std::size_t action_count = static_cast<std::size_t>(action::count);

constexpr std::size_t index_of(action value)
{
  return static_cast<std::size_t>(value);
}

enum class input_device
{
  keyboard,
  mouse,
  gamepad
};

// A physical control. `code` is whatever the platform layer uses to identify
// it, kept opaque here so this header never depends on raylib.
struct binding
{
  input_device device = input_device::keyboard;
  int code = 0;

  friend bool operator==(binding, binding) = default;
};

} // namespace arpg
