// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/default_bindings.hpp"
#include "core/input_snapshot.hpp"

#include <vector>

namespace arpg
{

class pixel_canvas;

// Reads raylib and fills a snapshot. This is the only place in the input path
// that knows a backend exists; everything downstream works on the snapshot.
class raylib_input
{
public:
  // Stick magnitude below which the pad is considered at rest.
  static constexpr float stick_deadzone = 0.2f;

  // raylib key and button constants, handed to the binding tables so they
  // never include raylib themselves.
  static control_codes codes();

  // Polls the given controls once, for the current rendered frame. The mouse
  // position is reported in canvas pixels.
  void sample(const std::vector<binding>& watched, const pixel_canvas& canvas, input_snapshot& out) const;
};

} // namespace arpg
