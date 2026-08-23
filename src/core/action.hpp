// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstddef>

namespace arpg
{

/// What the gameplay reacts to.
///
/// Nothing outside the input layer ever mentions a key or a button, so
/// remapping and supporting AZERTY next to QWERTY are a matter of bindings
/// rather than of code.
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
  count ///< Number of actions, not an action itself.
};

/// Number of values in arpg::action, for sizing containers indexed by one.
inline constexpr std::size_t action_count = static_cast<std::size_t>(action::count);

/// Index of @p value, for use with action_set.
constexpr std::size_t index_of(action value)
{
  return static_cast<std::size_t>(value);
}

/// Kind of device a binding belongs to.
enum class input_device
{
  keyboard,
  mouse,
  gamepad
};

/// A physical control: a key, a mouse button or a gamepad button.
///
/// @c code is whatever the platform layer uses to identify it, kept opaque so
/// this header never has to include the backend.
struct binding
{
  input_device device = input_device::keyboard; ///< Which device the code belongs to.
  int code = 0; ///< Backend identifier; the same number means different controls on different devices.

  /// Two bindings match only when both the device and the code do.
  friend bool operator==(binding, binding) = default;
};

} // namespace arpg
