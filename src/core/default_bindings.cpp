// SPDX-License-Identifier: BSL-1.0

#include <core/default_bindings.hpp>

namespace arpg
{

namespace
{

binding key(int code)
{
  return binding{input_device::keyboard, code};
}

binding pad(int code)
{
  return binding{input_device::gamepad, code};
}

binding mouse(int code)
{
  return binding{input_device::mouse, code};
}

} // namespace

action_map dungeon_bindings(const control_codes& codes)
{
  action_map map;

  map.bind(key(codes.key_w), action::move_up);
  map.bind(key(codes.key_up), action::move_up);
  map.bind(pad(codes.pad_up), action::move_up);

  map.bind(key(codes.key_s), action::move_down);
  map.bind(key(codes.key_down), action::move_down);
  map.bind(pad(codes.pad_down), action::move_down);

  map.bind(key(codes.key_a), action::move_left);
  map.bind(key(codes.key_left), action::move_left);
  map.bind(pad(codes.pad_left), action::move_left);

  map.bind(key(codes.key_d), action::move_right);
  map.bind(key(codes.key_right), action::move_right);
  map.bind(pad(codes.pad_right), action::move_right);

  map.bind(mouse(codes.mouse_left), action::shoot);
  map.bind(pad(codes.pad_right_trigger), action::shoot);

  map.bind(key(codes.key_space), action::dash);
  map.bind(pad(codes.pad_south), action::dash);

  map.bind(key(codes.key_shift), action::focus);
  map.bind(pad(codes.pad_left_shoulder), action::focus);

  map.bind(key(codes.key_escape), action::pause);
  map.bind(pad(codes.pad_start), action::pause);

  return map;
}

action_map menu_bindings(const control_codes& codes)
{
  action_map map;

  map.bind(key(codes.key_w), action::menu_up);
  map.bind(key(codes.key_up), action::menu_up);
  map.bind(pad(codes.pad_up), action::menu_up);

  map.bind(key(codes.key_s), action::menu_down);
  map.bind(key(codes.key_down), action::menu_down);
  map.bind(pad(codes.pad_down), action::menu_down);

  map.bind(key(codes.key_a), action::menu_left);
  map.bind(key(codes.key_left), action::menu_left);
  map.bind(pad(codes.pad_left), action::menu_left);

  map.bind(key(codes.key_d), action::menu_right);
  map.bind(key(codes.key_right), action::menu_right);
  map.bind(pad(codes.pad_right), action::menu_right);

  map.bind(key(codes.key_enter), action::confirm);
  map.bind(key(codes.key_space), action::confirm);
  map.bind(pad(codes.pad_south), action::confirm);

  map.bind(key(codes.key_escape), action::cancel);
  map.bind(pad(codes.pad_east), action::cancel);

  return map;
}

} // namespace arpg
