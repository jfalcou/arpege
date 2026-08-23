// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/action.hpp"
#include "core/vec2.hpp"

#include <algorithm>
#include <vector>

namespace arpg
{

// Raw state of one rendered frame, filled by the platform layer and consumed by
// the simulation steps. Holds no key semantics, only what is physically down.
struct input_snapshot
{
  std::vector<binding> down;
  vec2 left_stick{};
  vec2 right_stick{};

  // Mouse position expressed in canvas pixels, already mapped by the caller.
  vec2 mouse_position{};
  bool mouse_moved = false;
  bool gamepad_present = false;

  // Device that last said something, so the UI can show matching prompts.
  // Derived by the application, not read from the hardware.
  input_device device = input_device::keyboard;

  bool holds(binding control) const { return std::find(down.begin(), down.end(), control) != down.end(); }

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
