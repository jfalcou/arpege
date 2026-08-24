// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <core/default_bindings.hpp>

namespace
{

// Distinct values so a mix-up between two codes shows up as a failure rather
// than silently matching.
arpg::control_codes test_codes()
{
  arpg::control_codes codes;

  codes.key_w = 1;
  codes.key_a = 2;
  codes.key_s = 3;
  codes.key_d = 4;
  codes.key_up = 5;
  codes.key_down = 6;
  codes.key_left = 7;
  codes.key_right = 8;
  codes.key_space = 9;
  codes.key_shift = 10;
  codes.key_enter = 11;
  codes.key_escape = 12;
  codes.mouse_left = 13;
  codes.pad_up = 14;
  codes.pad_down = 15;
  codes.pad_left = 16;
  codes.pad_right = 17;
  codes.pad_south = 18;
  codes.pad_east = 19;
  codes.pad_right_trigger = 20;
  codes.pad_left_shoulder = 21;
  codes.pad_start = 22;

  return codes;
}

bool triggers(const arpg::action_map& map, arpg::binding control, arpg::action target)
{
  arpg::input_snapshot snapshot;
  snapshot.gamepad_present = true;
  snapshot.down = {control};

  return map.resolve(snapshot).test(arpg::index_of(target));
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

TTS_CASE("Dungeon movement answers to WASD, to the arrows and to the pad")
{
  const auto codes = test_codes();
  const auto map = arpg::dungeon_bindings(codes);

  TTS_EXPECT(triggers(map, key(codes.key_w), arpg::action::move_up));
  TTS_EXPECT(triggers(map, key(codes.key_up), arpg::action::move_up));
  TTS_EXPECT(triggers(map, pad(codes.pad_up), arpg::action::move_up));

  TTS_EXPECT(triggers(map, key(codes.key_s), arpg::action::move_down));
  TTS_EXPECT(triggers(map, key(codes.key_a), arpg::action::move_left));
  TTS_EXPECT(triggers(map, key(codes.key_d), arpg::action::move_right));
  TTS_EXPECT(triggers(map, key(codes.key_left), arpg::action::move_left));
  TTS_EXPECT(triggers(map, key(codes.key_right), arpg::action::move_right));
};

TTS_CASE("Every dungeon action is reachable")
{
  const auto codes = test_codes();
  const auto map = arpg::dungeon_bindings(codes);

  for (const auto target :
       {arpg::action::move_up, arpg::action::move_down, arpg::action::move_left, arpg::action::move_right,
        arpg::action::shoot, arpg::action::dash, arpg::action::focus, arpg::action::pause})
  {
    TTS_EXPECT(map.controls_for(target).size() > 0U);
  }
};

TTS_CASE("Every dungeon action is reachable without a pad")
{
  const auto codes = test_codes();
  const auto map = arpg::dungeon_bindings(codes);

  // A keyboard-only player must not find an action they cannot trigger.
  for (const auto target :
       {arpg::action::move_up, arpg::action::move_down, arpg::action::move_left, arpg::action::move_right,
        arpg::action::shoot, arpg::action::dash, arpg::action::focus, arpg::action::pause})
  {
    bool without_pad = false;
    for (const auto control : map.controls_for(target))
    {
      without_pad = without_pad || control.device != arpg::input_device::gamepad;
    }
    TTS_EXPECT(without_pad);
  }
};

TTS_CASE("Every menu action is reachable")
{
  const auto codes = test_codes();
  const auto map = arpg::menu_bindings(codes);

  for (const auto target : {arpg::action::menu_up, arpg::action::menu_down, arpg::action::menu_left,
                            arpg::action::menu_right, arpg::action::confirm, arpg::action::cancel})
  {
    TTS_EXPECT(map.controls_for(target).size() > 0U);
  }
};

TTS_CASE("The same key means different things in a menu and in a dungeon")
{
  const auto codes = test_codes();
  const auto dungeon = arpg::dungeon_bindings(codes);
  const auto menu = arpg::menu_bindings(codes);

  TTS_EXPECT(triggers(dungeon, key(codes.key_w), arpg::action::move_up));
  TTS_EXPECT_NOT(triggers(menu, key(codes.key_w), arpg::action::move_up));
  TTS_EXPECT(triggers(menu, key(codes.key_w), arpg::action::menu_up));

  // Escape pauses a run but backs out of a menu.
  TTS_EXPECT(triggers(dungeon, key(codes.key_escape), arpg::action::pause));
  TTS_EXPECT(triggers(menu, key(codes.key_escape), arpg::action::cancel));
};

TTS_CASE("A menu action never leaks into the dungeon map")
{
  const auto codes = test_codes();
  const auto dungeon = arpg::dungeon_bindings(codes);

  for (const auto target : {arpg::action::menu_up, arpg::action::confirm, arpg::action::cancel})
  {
    TTS_EQUAL(dungeon.controls_for(target).size(), 0U);
  }
};

TTS_CASE("Shooting is on the mouse and on a trigger")
{
  const auto codes = test_codes();
  const auto map = arpg::dungeon_bindings(codes);

  TTS_EXPECT(triggers(map, arpg::binding{arpg::input_device::mouse, codes.mouse_left}, arpg::action::shoot));
  TTS_EXPECT(triggers(map, pad(codes.pad_right_trigger), arpg::action::shoot));
};

TTS_CASE("The polled control list has no duplicates")
{
  const auto codes = test_codes();
  const auto map = arpg::menu_bindings(codes);

  // Space confirms and is also bound elsewhere; polling it twice would be
  // wasteful and would report it twice in the snapshot.
  const auto controls = map.controls();

  for (std::size_t i = 0; i < controls.size(); ++i)
  {
    for (std::size_t j = i + 1; j < controls.size(); ++j)
    {
      TTS_EXPECT_NOT(controls[i] == controls[j]);
    }
  }

  TTS_EXPECT(controls.size() > 0U);
};

TTS_CASE("Clearing a map disables everything")
{
  const auto codes = test_codes();
  auto map = arpg::dungeon_bindings(codes);

  map.clear();

  TTS_EQUAL(map.size(), 0U);
  TTS_EXPECT_NOT(triggers(map, key(codes.key_w), arpg::action::move_up));
};
