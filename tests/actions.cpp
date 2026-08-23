// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include "core/action_map.hpp"
#include "core/action_state.hpp"

namespace
{

// Stand-in key codes: the layer treats them as opaque, so the tests need no
// raylib constant.
constexpr int code_w = 87;
constexpr int code_up = 265;
constexpr int code_pad_up = 1;
constexpr int code_escape = 256;

arpg::input_snapshot with(std::initializer_list<arpg::binding> down)
{
  arpg::input_snapshot snapshot;
  snapshot.down = down;
  return snapshot;
}

arpg::binding key(int code)
{
  return arpg::binding{arpg::input_device::keyboard, code};
}

arpg::binding pad(int code)
{
  return arpg::binding{arpg::input_device::gamepad, code};
}

} // namespace

TTS_CASE("An unbound control triggers nothing")
{
  arpg::action_map map;
  map.bind(key(code_w), arpg::action::move_up);

  const auto held = map.resolve(with({key(code_escape)}));
  TTS_EXPECT_NOT(held.test(arpg::index_of(arpg::action::move_up)));
  TTS_EQUAL(held.count(), 0U);
};

TTS_CASE("Several controls can drive the same action")
{
  arpg::action_map map;
  map.bind(key(code_w), arpg::action::move_up);
  map.bind(key(code_up), arpg::action::move_up);
  map.bind(pad(code_pad_up), arpg::action::move_up);

  for (const auto control : {key(code_w), key(code_up), pad(code_pad_up)})
  {
    TTS_EXPECT(map.resolve(with({control})).test(arpg::index_of(arpg::action::move_up)));
  }

  TTS_EQUAL(map.controls_for(arpg::action::move_up).size(), 3U);
};

TTS_CASE("The same code on two devices stays distinct")
{
  arpg::action_map map;
  map.bind(key(code_pad_up), arpg::action::move_up);

  // Same numeric code, gamepad instead of keyboard: must not match.
  TTS_EXPECT_NOT(map.resolve(with({pad(code_pad_up)})).test(arpg::index_of(arpg::action::move_up)));
};

TTS_CASE("Unbinding removes every control of that action")
{
  arpg::action_map map;
  map.bind(key(code_w), arpg::action::move_up);
  map.bind(key(code_up), arpg::action::move_up);
  map.bind(key(code_escape), arpg::action::pause);

  map.unbind(arpg::action::move_up);

  TTS_EQUAL(map.controls_for(arpg::action::move_up).size(), 0U);
  TTS_EXPECT(map.resolve(with({key(code_escape)})).test(arpg::index_of(arpg::action::pause)));
};

TTS_CASE("Keys give eight directions, and a diagonal is not faster")
{
  arpg::action_set held;
  held.set(arpg::index_of(arpg::action::move_right));

  const auto right = arpg::movement_direction(held, arpg::vec2{});
  TTS_EQUAL(right.x, 1.0f);
  TTS_EQUAL(right.y, 0.0f);

  held.set(arpg::index_of(arpg::action::move_down));
  const auto diagonal = arpg::movement_direction(held, arpg::vec2{});
  TTS_ULP_EQUAL(arpg::length(diagonal), 1.0f, 4.0);
};

TTS_CASE("Opposite keys cancel out")
{
  arpg::action_set held;
  held.set(arpg::index_of(arpg::action::move_left));
  held.set(arpg::index_of(arpg::action::move_right));

  const auto direction = arpg::movement_direction(held, arpg::vec2{});
  TTS_EQUAL(direction.x, 0.0f);
  TTS_EQUAL(direction.y, 0.0f);
};

TTS_CASE("The stick wins over the keys and keeps its magnitude")
{
  arpg::action_set held;
  held.set(arpg::index_of(arpg::action::move_left));

  // A half pushed stick must stay half pushed, not be snapped to a key
  // direction.
  const auto direction = arpg::movement_direction(held, arpg::vec2{0.0f, 0.5f});
  TTS_EQUAL(direction.x, 0.0f);
  TTS_EQUAL(direction.y, 0.5f);
};

TTS_CASE("Menu navigation uses the menu actions, not the dungeon ones")
{
  arpg::action_set held;
  held.set(arpg::index_of(arpg::action::menu_up));

  // A menu binds its keys to menu_up and friends, so reading move_up there
  // would leave the keyboard doing nothing at all.
  const auto navigating = arpg::movement_direction(held, arpg::vec2{}, arpg::menu_actions);
  TTS_EQUAL(navigating.y, -1.0f);

  const auto moving = arpg::movement_direction(held, arpg::vec2{});
  TTS_EQUAL(moving.x, 0.0f);
  TTS_EQUAL(moving.y, 0.0f);
};

TTS_CASE("Both direction sets read their own actions")
{
  arpg::action_set held;
  held.set(arpg::index_of(arpg::action::move_right));

  TTS_EQUAL(arpg::movement_direction(held, arpg::vec2{}).x, 1.0f);
  TTS_EQUAL(arpg::movement_direction(held, arpg::vec2{}, arpg::menu_actions).x, 0.0f);
};

TTS_CASE("The active device follows whoever last said something")
{
  arpg::input_snapshot snapshot;
  snapshot.gamepad_present = true;
  snapshot.left_stick = arpg::vec2{0.5f, 0.0f};
  TTS_EQUAL(arpg::active_device(snapshot, arpg::input_device::keyboard), arpg::input_device::gamepad);

  snapshot.clear();
  snapshot.down = {key(code_w)};
  TTS_EQUAL(arpg::active_device(snapshot, arpg::input_device::gamepad), arpg::input_device::keyboard);

  snapshot.clear();
  snapshot.mouse_moved = true;
  TTS_EQUAL(arpg::active_device(snapshot, arpg::input_device::keyboard), arpg::input_device::mouse);
};

TTS_CASE("An idle frame keeps the previous device")
{
  arpg::input_snapshot snapshot;
  TTS_EQUAL(arpg::active_device(snapshot, arpg::input_device::gamepad), arpg::input_device::gamepad);
};

TTS_CASE("A disconnected pad cannot claim the device")
{
  arpg::input_snapshot snapshot;
  snapshot.gamepad_present = false;
  snapshot.down = {pad(code_pad_up)};

  TTS_EQUAL(arpg::active_device(snapshot, arpg::input_device::keyboard), arpg::input_device::keyboard);
};

TTS_CASE("Aim is a position on mouse and a direction on pad")
{
  arpg::input_snapshot snapshot;
  snapshot.mouse_position = arpg::vec2{120.0f, 40.0f};
  snapshot.right_stick = arpg::vec2{0.0f, 2.0f};

  const auto with_mouse = arpg::resolve_aim(snapshot, arpg::input_device::mouse);
  TTS_EXPECT(with_mouse.absolute);
  TTS_EQUAL(with_mouse.value.x, 120.0f);

  const auto with_pad = arpg::resolve_aim(snapshot, arpg::input_device::gamepad);
  TTS_EXPECT_NOT(with_pad.absolute);
  TTS_ULP_EQUAL(arpg::length(with_pad.value), 1.0f, 2.0);
};
