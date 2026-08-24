// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/action.hpp>
#include <core/vec2.hpp>

#include <algorithm>
#include <vector>

namespace arpg
{

/// Raw input state of one rendered frame.
///
/// Filled by the platform layer and read by every simulation step of that
/// frame, which is what keeps a fixed timestep deterministic. Holds no key
/// semantics, only what is physically down.
struct input_snapshot
{
  /// Controls held down this frame.
  std::vector<binding> down;
  vec2 left_stick{};  ///< Already filtered through the deadzone by the caller.
  vec2 right_stick{}; ///< Already filtered through the deadzone by the caller.

  /// Mouse position in canvas pixels, already mapped by the caller.
  vec2 mouse_position{};
  bool mouse_moved = false;     ///< Moved during this frame, used to spot the active device.
  bool gamepad_present = false; ///< A pad is connected, so its controls can be trusted.

  /// Device that last said something, so prompts can match the controller in
  /// hand. Derived by the application rather than read from the hardware.
  input_device device = input_device::keyboard;

  /// Whether @p control is held down this frame.
  bool holds(binding control) const { return std::find(down.begin(), down.end(), control) != down.end(); }

  /// Forgets the frame, keeping only #device.
  void clear()
  {
    down.clear();
    left_stick = vec2{};
    right_stick = vec2{};
    mouse_moved = false;
    // device is deliberately kept: it must survive an idle frame.
  }
};

} // namespace arpg
