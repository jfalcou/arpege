// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/action_map.hpp"

namespace arpg
{

// Key codes are passed in rather than hardcoded, because the values belong to
// the platform layer and this one must not include raylib. The application
// fills the struct with the raylib constants.
struct control_codes
{
  int key_w = 0;
  int key_a = 0;
  int key_s = 0;
  int key_d = 0;
  int key_up = 0;
  int key_down = 0;
  int key_left = 0;
  int key_right = 0;
  int key_space = 0;
  int key_shift = 0;
  int key_enter = 0;
  int key_escape = 0;

  int mouse_left = 0;

  int pad_up = 0;
  int pad_down = 0;
  int pad_left = 0;
  int pad_right = 0;
  int pad_south = 0;
  int pad_east = 0;
  int pad_right_trigger = 0;
  int pad_left_shoulder = 0;
  int pad_start = 0;
};

// WASD and arrows both move, so QWERTY and AZERTY are both playable before any
// remapping.
action_map dungeon_bindings(const control_codes& codes);
action_map menu_bindings(const control_codes& codes);

} // namespace arpg
