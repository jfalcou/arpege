// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/action_map.hpp"

namespace arpg
{

/// Platform key and button codes needed to build the default bindings.
///
/// They are passed in rather than hardcoded, because the values belong to the
/// platform layer while this one must stay free of it. The application fills
/// the struct with the constants of the backend.
struct control_codes
{
  /// @name Keyboard
  /// WASD and the arrows are both bound, which is what makes AZERTY playable
  /// without remapping anything.
  /// @{
  int key_w = 0;      ///< W, or Z on an AZERTY layout.
  int key_a = 0;      ///< A, or Q on an AZERTY layout.
  int key_s = 0;      ///< S.
  int key_d = 0;      ///< D.
  int key_up = 0;     ///< Up arrow.
  int key_down = 0;   ///< Down arrow.
  int key_left = 0;   ///< Left arrow.
  int key_right = 0;  ///< Right arrow.
  int key_space = 0;  ///< Space bar.
  int key_shift = 0;  ///< Left shift.
  int key_enter = 0;  ///< Enter.
  int key_escape = 0; ///< Escape, which pauses a run but backs out of a menu.
  /// @}

  int mouse_left = 0; ///< Left mouse button.

  /// @name Gamepad
  /// Named by position rather than by letter, since the labels differ between
  /// controller families.
  /// @{
  int pad_up = 0;            ///< D-pad up.
  int pad_down = 0;          ///< D-pad down.
  int pad_left = 0;          ///< D-pad left.
  int pad_right = 0;         ///< D-pad right.
  int pad_south = 0;         ///< Bottom face button: A on Xbox, cross on PlayStation.
  int pad_east = 0;          ///< Right face button: B on Xbox, circle on PlayStation.
  int pad_right_trigger = 0; ///< Right trigger.
  int pad_left_shoulder = 0; ///< Left shoulder button.
  int pad_start = 0;         ///< Start, or whatever the middle-right button is called.
  /// @}
};

/// Bindings for a dungeon: movement, shooting, dash, focus and pause.
///
/// WASD and the arrows both move, so QWERTY and AZERTY are playable before any
/// remapping.
action_map dungeon_bindings(const control_codes& codes);

/// Bindings for a menu: navigation, confirm and cancel.
action_map menu_bindings(const control_codes& codes);

} // namespace arpg
