// SPDX-License-Identifier: BSL-1.0

#include <core/raylib_input.hpp>

#include <core/deadzone.hpp>
#include <core/pixel_canvas.hpp>

#include <raylib.h>

namespace arpg
{

namespace
{

// The engine only ever reads the first pad; splitscreen is not on the table.
constexpr int pad_slot = 0;

bool is_down(const binding& control, bool gamepad_present)
{
  switch (control.device)
  {
  case input_device::keyboard:
    return IsKeyDown(control.code);
  case input_device::mouse:
    return IsMouseButtonDown(control.code);
  case input_device::gamepad:
    return gamepad_present && IsGamepadButtonDown(pad_slot, control.code);
  }

  return false;
}

vec2 stick(int axis_x, int axis_y)
{
  return vec2{GetGamepadAxisMovement(pad_slot, axis_x), GetGamepadAxisMovement(pad_slot, axis_y)};
}

} // namespace

control_codes raylib_input::codes()
{
  control_codes result;

  result.key_w = KEY_W;
  result.key_a = KEY_A;
  result.key_s = KEY_S;
  result.key_d = KEY_D;
  result.key_up = KEY_UP;
  result.key_down = KEY_DOWN;
  result.key_left = KEY_LEFT;
  result.key_right = KEY_RIGHT;
  result.key_space = KEY_SPACE;
  result.key_shift = KEY_LEFT_SHIFT;
  result.key_enter = KEY_ENTER;
  result.key_escape = KEY_ESCAPE;

  result.mouse_left = MOUSE_BUTTON_LEFT;

  result.pad_up = GAMEPAD_BUTTON_LEFT_FACE_UP;
  result.pad_down = GAMEPAD_BUTTON_LEFT_FACE_DOWN;
  result.pad_left = GAMEPAD_BUTTON_LEFT_FACE_LEFT;
  result.pad_right = GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
  result.pad_south = GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
  result.pad_east = GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;
  result.pad_right_trigger = GAMEPAD_BUTTON_RIGHT_TRIGGER_2;
  result.pad_left_shoulder = GAMEPAD_BUTTON_LEFT_TRIGGER_1;
  result.pad_start = GAMEPAD_BUTTON_MIDDLE_RIGHT;

  return result;
}

void raylib_input::sample(const std::vector<binding>& watched, const pixel_canvas& canvas, input_snapshot& out) const
{
  out.clear();
  out.gamepad_present = IsGamepadAvailable(pad_slot);

  for (const binding& control : watched)
  {
    if (is_down(control, out.gamepad_present))
    {
      out.down.push_back(control);
    }
  }

  if (out.gamepad_present)
  {
    out.left_stick = apply_radial_deadzone(stick(GAMEPAD_AXIS_LEFT_X, GAMEPAD_AXIS_LEFT_Y), stick_deadzone);
    out.right_stick = apply_radial_deadzone(stick(GAMEPAD_AXIS_RIGHT_X, GAMEPAD_AXIS_RIGHT_Y), stick_deadzone);
  }

  const Vector2 in_canvas = canvas.screen_to_canvas(GetMousePosition());
  out.mouse_position = vec2{in_canvas.x, in_canvas.y};

  const Vector2 delta = GetMouseDelta();
  out.mouse_moved = delta.x != 0.0f || delta.y != 0.0f;
}

} // namespace arpg
