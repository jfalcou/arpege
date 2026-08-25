// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <ecs/dash.hpp>

namespace
{

constexpr arpg::dash_profile profile{.speed = 200.0f, .duration = 0.2f, .cooldown = 0.6f, .mercy = 0.2f};
constexpr float step = 1.0f / 60.0f;

const arpg::vec2 east{1.0f, 0.0f};
const arpg::vec2 north{0.0f, -1.0f};

/// Runs the dash forward without asking for a new one, one step past the
/// asked-for time: a fixed step lands on the boundary itself, where what is
/// left of a timer is a rounding error rather than a duration.
void coast(arpg::dash_state& dash, float seconds)
{
  const int steps = static_cast<int>(seconds / step) + 1;

  for (int taken = 0; taken < steps; ++taken)
  {
    arpg::advance_dash(dash, profile, false, arpg::vec2{}, step);
  }
}

} // namespace

TTS_CASE("A dash asked for while moving leaves along that direction")
{
  arpg::dash_state dash;

  TTS_EXPECT(arpg::advance_dash(dash, profile, true, east, step));
  TTS_EXPECT(arpg::dashing(dash));
  TTS_EQUAL(arpg::dash_velocity(dash, profile).x, 200.0f);
};

TTS_CASE("A dash keeps its heading even when the steering turns")
{
  arpg::dash_state dash;
  arpg::advance_dash(dash, profile, true, east, step);

  // Committing to the direction is what makes a dash a decision rather than a
  // faster way to walk.
  arpg::advance_dash(dash, profile, false, north, step);

  TTS_EQUAL(arpg::dash_velocity(dash, profile).x, 200.0f);
  TTS_EQUAL(arpg::dash_velocity(dash, profile).y, 0.0f);
};

TTS_CASE("A dash ends after its duration")
{
  arpg::dash_state dash;
  arpg::advance_dash(dash, profile, true, east, step);

  coast(dash, profile.duration);

  TTS_EXPECT_NOT(arpg::dashing(dash));
  TTS_EQUAL(arpg::length_squared(arpg::dash_velocity(dash, profile)), 0.0f);
};

TTS_CASE("A second dash cannot start during the first")
{
  arpg::dash_state dash;
  arpg::advance_dash(dash, profile, true, east, step);

  TTS_EXPECT_NOT(arpg::advance_dash(dash, profile, true, north, step));
};

TTS_CASE("A dash waits for its cooldown, counted from the start of the last one")
{
  arpg::dash_state dash;
  arpg::advance_dash(dash, profile, true, east, step);

  coast(dash, profile.duration);
  TTS_EXPECT_NOT(arpg::advance_dash(dash, profile, true, east, step));

  // The figure means the same whatever the duration is, so what is left to
  // wait is the cooldown minus the dash itself.
  coast(dash, profile.cooldown - profile.duration);
  TTS_EXPECT(arpg::advance_dash(dash, profile, true, east, step));
};

TTS_CASE("A dash asked for while standing still goes the last way asked for")
{
  arpg::dash_state dash;

  arpg::advance_dash(dash, profile, false, north, step);
  TTS_EXPECT(arpg::advance_dash(dash, profile, true, arpg::vec2{}, step));

  TTS_EQUAL(arpg::dash_velocity(dash, profile).y, -200.0f);
};

TTS_CASE("A player who has never moved cannot spend a dash on nothing")
{
  arpg::dash_state dash;

  TTS_EXPECT_NOT(arpg::advance_dash(dash, profile, true, arpg::vec2{}, step));
  TTS_EXPECT_NOT(arpg::dashing(dash));
  TTS_EQUAL(dash.cooldown, 0.0f);
};

TTS_CASE("A dash carries at its own speed whatever the steering was worth")
{
  arpg::dash_state dash;

  // Steering comes in normalised, but a half-held stick must not buy half a
  // dash.
  arpg::advance_dash(dash, profile, true, arpg::vec2{3.0f, 4.0f}, step);

  TTS_ULP_EQUAL(arpg::length_squared(arpg::dash_velocity(dash, profile)), 200.0f * 200.0f, 8.0);
};

TTS_CASE("A duration longer than the cooldown does not overlap the next dash")
{
  constexpr arpg::dash_profile slow{.speed = 100.0f, .duration = 0.5f, .cooldown = 0.2f, .mercy = 0.5f};
  arpg::dash_state dash;

  arpg::advance_dash(dash, slow, true, east, step);

  // A dash that outlives its own cooldown would otherwise be restartable while
  // still running, and the state would be two dashes at once.
  TTS_EXPECT(dash.cooldown >= slow.duration);
};
