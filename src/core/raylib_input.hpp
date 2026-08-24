// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/default_bindings.hpp"
#include "core/input_snapshot.hpp"

#include <vector>

namespace arpg
{

class pixel_canvas;

/// Reads the backend and fills an input_snapshot.
///
/// The only place in the input path that knows a backend exists; everything
/// downstream works on the snapshot alone.
class raylib_input
{
public:
  /// Stick magnitude below which the pad counts as resting.
  static constexpr float stick_deadzone = 0.2f;

  /// Backend key and button constants, handed to the binding tables so they
  /// never have to include the backend themselves.
  static control_codes codes();

  /// Polls @p watched once, for the current rendered frame.
  ///
  /// Only bound controls are polled, which avoids walking the hundreds of key
  /// codes the backend knows about. The mouse position lands in @p out already
  /// expressed in canvas pixels.
  void sample(const std::vector<binding>& watched, const pixel_canvas& canvas, input_snapshot& out) const;
};

} // namespace arpg
