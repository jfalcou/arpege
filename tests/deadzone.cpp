// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include "core/deadzone.hpp"

TTS_CASE("Anything inside the deadzone reads as neutral")
{
  const auto centre = arpg::apply_radial_deadzone(arpg::vec2{0.0f, 0.0f}, 0.25f);
  TTS_EQUAL(centre.x, 0.0f);
  TTS_EQUAL(centre.y, 0.0f);

  const auto drift = arpg::apply_radial_deadzone(arpg::vec2{0.1f, 0.1f}, 0.25f);
  TTS_EQUAL(drift.x, 0.0f);
  TTS_EQUAL(drift.y, 0.0f);
};

TTS_CASE("A fully pushed stick reads as a unit vector")
{
  const auto right = arpg::apply_radial_deadzone(arpg::vec2{1.0f, 0.0f}, 0.25f);
  TTS_ULP_EQUAL(arpg::length(right), 1.0f, 2.0);

  const auto beyond = arpg::apply_radial_deadzone(arpg::vec2{2.0f, 0.0f}, 0.25f);
  TTS_ULP_EQUAL(arpg::length(beyond), 1.0f, 2.0);
};

TTS_CASE("The magnitude restarts from zero at the deadzone edge")
{
  // Just outside the deadzone the stick must be nearly neutral, not jump to
  // the deadzone value.
  const auto edge = arpg::apply_radial_deadzone(arpg::vec2{0.26f, 0.0f}, 0.25f);
  TTS_EXPECT(arpg::length(edge) < 0.05f);
  TTS_EXPECT(arpg::length(edge) > 0.0f);

  // Halfway between the edge and the rim gives half the magnitude.
  const auto middle = arpg::apply_radial_deadzone(arpg::vec2{0.625f, 0.0f}, 0.25f);
  TTS_ULP_EQUAL(arpg::length(middle), 0.5f, 8.0);
};

TTS_CASE("The deadzone is radial, not per axis")
{
  // Both components sit below the deadzone but their magnitude does not, so a
  // per axis filter would wrongly reject this diagonal.
  const arpg::vec2 diagonal{0.2f, 0.2f};
  TTS_EXPECT(arpg::length(diagonal) > 0.25f);

  const auto filtered = arpg::apply_radial_deadzone(diagonal, 0.25f);
  TTS_EXPECT(arpg::length(filtered) > 0.0f);
};

TTS_CASE("Filtering preserves the direction")
{
  const auto filtered = arpg::apply_radial_deadzone(arpg::vec2{0.6f, 0.8f}, 0.25f);

  // Same heading as the raw stick, only the magnitude changed.
  TTS_ULP_EQUAL(filtered.x / arpg::length(filtered), 0.6f, 8.0);
  TTS_ULP_EQUAL(filtered.y / arpg::length(filtered), 0.8f, 8.0);
};

TTS_CASE("A degenerate range never divides by zero")
{
  const auto same = arpg::apply_radial_deadzone(arpg::vec2{1.0f, 0.0f}, 0.5f, 0.5f);
  TTS_EQUAL(same.x, 0.0f);
  TTS_EQUAL(same.y, 0.0f);
};
