// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <core/action_state.hpp>

namespace
{

arpg::action_set only(arpg::action target)
{
  arpg::action_set set;
  set.set(arpg::index_of(target));
  return set;
}

/// A state that has already seen one step with nothing down.
///
/// Nothing is a press on the very first step, since what is down then was
/// pressed before the state existed. These tests are about what the buffer
/// does with a press, so they start after that.
arpg::action_state ready()
{
  arpg::action_state state;
  state.advance(arpg::action_set{});
  return state;
}

} // namespace

TTS_CASE("Nothing is claimable before anything is pressed")
{
  arpg::action_state state;

  // Guards against a zeroed buffer reading as "just pressed".
  TTS_EXPECT_NOT(state.consume(arpg::action::dash));

  state.advance(arpg::action_set{});
  TTS_EXPECT_NOT(state.consume(arpg::action::dash));
  TTS_EXPECT_NOT(state.held(arpg::action::dash));
};

TTS_CASE("A press is an edge, holding is not")
{
  arpg::action_state state = ready();

  state.advance(only(arpg::action::dash));
  TTS_EXPECT(state.pressed(arpg::action::dash));
  TTS_EXPECT(state.held(arpg::action::dash));

  state.advance(only(arpg::action::dash));
  TTS_EXPECT_NOT(state.pressed(arpg::action::dash));
  TTS_EXPECT(state.held(arpg::action::dash));

  state.advance(arpg::action_set{});
  TTS_EXPECT(state.released(arpg::action::dash));
  TTS_EXPECT_NOT(state.held(arpg::action::dash));
};

TTS_CASE("A press stays claimable for a few steps after release")
{
  arpg::action_state state = ready();
  state.advance(only(arpg::action::dash));

  // Four steps go by with the button already released.
  for (int i = 0; i < 4; ++i)
  {
    state.advance(arpg::action_set{});
  }

  TTS_EXPECT(state.consume(arpg::action::dash));
};

TTS_CASE("An old press expires")
{
  arpg::action_state state = ready();
  state.advance(only(arpg::action::dash));

  for (int i = 0; i < arpg::action_state::default_buffer_steps + 2; ++i)
  {
    state.advance(arpg::action_set{});
  }

  TTS_EXPECT_NOT(state.consume(arpg::action::dash));
};

TTS_CASE("A press is claimed once and only once")
{
  arpg::action_state state = ready();
  state.advance(only(arpg::action::dash));

  TTS_EXPECT(state.consume(arpg::action::dash));
  TTS_EXPECT_NOT(state.consume(arpg::action::dash));
};

TTS_CASE("Claiming one action leaves the others alone")
{
  arpg::action_set both;
  both.set(arpg::index_of(arpg::action::dash));
  both.set(arpg::index_of(arpg::action::shoot));

  arpg::action_state state = ready();
  state.advance(both);

  TTS_EXPECT(state.consume(arpg::action::dash));
  TTS_EXPECT(state.consume(arpg::action::shoot));
};

TTS_CASE("A new press after a claim is claimable again")
{
  arpg::action_state state = ready();

  state.advance(only(arpg::action::dash));
  TTS_EXPECT(state.consume(arpg::action::dash));

  state.advance(arpg::action_set{});
  state.advance(only(arpg::action::dash));
  TTS_EXPECT(state.consume(arpg::action::dash));
};

TTS_CASE("A tighter window rejects a press the default would accept")
{
  arpg::action_state state = ready();
  state.advance(only(arpg::action::dash));

  for (int i = 0; i < 3; ++i)
  {
    state.advance(arpg::action_set{});
  }

  TTS_EXPECT_NOT(state.consume(arpg::action::dash, 1));
  TTS_EXPECT(state.consume(arpg::action::dash));
};

TTS_CASE("Flushing drops what was buffered")
{
  arpg::action_state state = ready();
  state.advance(only(arpg::action::dash));

  state.flush();

  TTS_EXPECT_NOT(state.consume(arpg::action::dash));
};
