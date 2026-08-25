// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <ecs/firing_pattern.hpp>

#include <cmath>

namespace
{

const arpg::vec2 east{1.0f, 0.0f};

/// Degrees between two headings, which is what a pattern is described in.
float angle_between(arpg::vec2 a, arpg::vec2 b)
{
  const float cosine = a.x * b.x + a.y * b.y;
  const float sine = a.x * b.y - a.y * b.x;
  return std::atan2(sine, cosine) * 180.0f / 3.14159265358979323846f;
}

float heading_of(arpg::vec2 v)
{
  return std::atan2(v.y, v.x) * 180.0f / 3.14159265358979323846f;
}

} // namespace

TTS_CASE("A single aimed bullet goes where it was pointed")
{
  const arpg::firing_pattern shot{};
  std::vector<arpg::vec2> headings;

  arpg::volley_headings(shot, east, 0, headings);

  TTS_EQUAL(headings.size(), 1U);
  TTS_ULP_EQUAL(headings[0].x, 1.0f, 2.0);
  TTS_ABSOLUTE_EQUAL(headings[0].y, 0.0f, 1e-6);
};

TTS_CASE("A fan opens around the heading rather than to one side")
{
  const arpg::firing_pattern shot{.bullets = 5, .arc = 40.0f};
  std::vector<arpg::vec2> headings;

  arpg::volley_headings(shot, east, 0, headings);

  TTS_EQUAL(headings.size(), 5U);

  // The middle one keeps the aim, and the two edges sit at half the arc.
  TTS_ABSOLUTE_EQUAL(heading_of(headings[2]), 0.0f, 1e-4);
  TTS_ABSOLUTE_EQUAL(heading_of(headings[0]), -20.0f, 1e-4);
  TTS_ABSOLUTE_EQUAL(heading_of(headings[4]), 20.0f, 1e-4);
};

TTS_CASE("A fan spaces its bullets evenly")
{
  const arpg::firing_pattern shot{.bullets = 4, .arc = 30.0f};
  std::vector<arpg::vec2> headings;

  arpg::volley_headings(shot, east, 0, headings);

  for (std::size_t i = 1; i < headings.size(); ++i)
  {
    TTS_ABSOLUTE_EQUAL(angle_between(headings[i - 1], headings[i]), 10.0f, 1e-4);
  }
};

TTS_CASE("A full turn does not fire twice along the same heading")
{
  const arpg::firing_pattern shot{.aim = arpg::aim_mode::fixed, .bullets = 8, .arc = 360.0f};
  std::vector<arpg::vec2> headings;

  arpg::volley_headings(shot, east, 0, headings);

  TTS_EQUAL(headings.size(), 8U);

  // Both ends of a full turn are the same direction, so sharing the arc
  // between the bullets rather than between the gaps is what keeps the last
  // one off the first.
  for (std::size_t i = 1; i < headings.size(); ++i)
  {
    TTS_ABSOLUTE_EQUAL(angle_between(headings[i - 1], headings[i]), 45.0f, 1e-3);
  }

  TTS_ABSOLUTE_EQUAL(std::abs(angle_between(headings.back(), headings.front())), 45.0f, 1e-3);
};

TTS_CASE("A spin turns the volley by the same step every time")
{
  const arpg::firing_pattern shot{.aim = arpg::aim_mode::fixed, .bullets = 1, .spin = 17.0f};
  std::vector<arpg::vec2> first;
  std::vector<arpg::vec2> tenth;

  arpg::volley_headings(shot, east, 0, first);
  arpg::volley_headings(shot, east, 10, tenth);

  TTS_ABSOLUTE_EQUAL(heading_of(first[0]), 0.0f, 1e-4);
  TTS_ABSOLUTE_EQUAL(angle_between(first[0], tenth[0]), 170.0f, 1e-3);
};

TTS_CASE("A fixed pattern ignores where the player stands")
{
  const arpg::firing_pattern shot{.aim = arpg::aim_mode::fixed, .bullets = 3, .arc = 60.0f};
  std::vector<arpg::vec2> east_facing;
  std::vector<arpg::vec2> north_facing;

  arpg::volley_headings(shot, east, 0, east_facing);
  arpg::volley_headings(shot, arpg::vec2{0.0f, 1.0f}, 0, north_facing);

  TTS_EQUAL(east_facing.size(), north_facing.size());
  for (std::size_t i = 0; i < east_facing.size(); ++i)
  {
    TTS_ABSOLUTE_EQUAL(heading_of(east_facing[i]), heading_of(north_facing[i]), 1e-4);
  }
};

TTS_CASE("An aimed pattern follows the player")
{
  const arpg::firing_pattern shot{.bullets = 3, .arc = 60.0f};
  std::vector<arpg::vec2> headings;

  arpg::volley_headings(shot, arpg::vec2{0.0f, 1.0f}, 0, headings);

  TTS_ABSOLUTE_EQUAL(heading_of(headings[1]), 90.0f, 1e-4);
};

TTS_CASE("Every heading comes out with a length of one")
{
  const arpg::firing_pattern shot{.bullets = 7, .arc = 210.0f, .spin = 13.0f};
  std::vector<arpg::vec2> headings;

  arpg::volley_headings(shot, arpg::vec2{3.0f, -4.0f}, 5, headings);

  for (const arpg::vec2 heading : headings)
  {
    TTS_ULP_EQUAL(arpg::length_squared(heading), 1.0f, 4.0);
  }
};

TTS_CASE("A pattern that fires nothing yields nothing")
{
  std::vector<arpg::vec2> headings{arpg::vec2{9.0f, 9.0f}};

  arpg::volley_headings(arpg::firing_pattern{.bullets = 0}, east, 0, headings);
  TTS_EXPECT(headings.empty());

  arpg::volley_headings(arpg::firing_pattern{.bullets = -3}, east, 0, headings);
  TTS_EXPECT(headings.empty());
};

TTS_CASE("Aiming at nothing fires nothing")
{
  std::vector<arpg::vec2> headings;

  // A target standing exactly on the shooter leaves no direction to fire
  // along, and a bullet with no heading would sit on the muzzle.
  arpg::volley_headings(arpg::firing_pattern{.bullets = 5, .arc = 40.0f}, arpg::vec2{}, 0, headings);

  TTS_EXPECT(headings.empty());
};

TTS_CASE("A fixed pattern still fires when the player is on top of it")
{
  std::vector<arpg::vec2> headings;

  arpg::volley_headings(arpg::firing_pattern{.aim = arpg::aim_mode::fixed, .bullets = 4, .arc = 360.0f},
                        arpg::vec2{}, 0, headings);

  TTS_EQUAL(headings.size(), 4U);
};
