// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <core/rng.hpp>

#include <array>
#include <vector>

TTS_CASE("The same seed replays the same run")
{
  arpg::rng first(1234);
  arpg::rng second(1234);

  for (int i = 0; i < 32; ++i)
  {
    TTS_EQUAL(first.next(), second.next());
  }
};

TTS_CASE("Neighbouring seeds do not start alike")
{
  arpg::rng first(1000);
  arpg::rng second(1001);

  // A generator seeded straight into its state would have these two open on
  // nearly the same value, and two players a seed apart would share a level.
  int identical = 0;
  for (int i = 0; i < 8; ++i)
  {
    identical += (first.next() == second.next()) ? 1 : 0;
  }

  TTS_EQUAL(identical, 0);
};

TTS_CASE("The stream is pinned, so a seed means the same thing everywhere")
{
  // Golden values, taken from this implementation. They are here to catch a
  // change to the algorithm, not to prove it correct: shared seeds and bug
  // reports carrying a seed both rely on this stream never moving.
  const std::array<std::uint32_t, 4> expected{0xC2F57BD6u, 0x6B07C4A9u, 0x72B7B29Bu, 0x44215383u};

  arpg::rng generator(42);

  for (const std::uint32_t value : expected)
  {
    TTS_EQUAL(generator.next(), value);
  }
};

TTS_CASE("A bounded draw stays inside its bound")
{
  arpg::rng generator(7);

  for (int i = 0; i < 512; ++i)
  {
    TTS_EXPECT(generator.below(6) < 6U);
  }
};

TTS_CASE("A bound of one always gives zero")
{
  arpg::rng generator(7);

  for (int i = 0; i < 16; ++i)
  {
    TTS_EQUAL(generator.below(1), 0U);
  }
};

TTS_CASE("A bound of zero returns zero instead of dividing by it")
{
  arpg::rng generator(7);
  TTS_EQUAL(generator.below(0), 0U);
};

TTS_CASE("Every value of the range comes up")
{
  arpg::rng generator(99);
  std::vector<int> seen(6, 0);

  for (int i = 0; i < 6000; ++i)
  {
    ++seen[generator.below(6)];
  }

  // A thousand expected each. The bounds are wide on purpose: this catches a
  // face that never appears or one that takes half the draws, not a wobble.
  for (const int count : seen)
  {
    TTS_EXPECT(count > 700);
    TTS_EXPECT(count < 1300);
  }
};

TTS_CASE("A unit draw stays in its interval")
{
  arpg::rng generator(2024);

  for (int i = 0; i < 4096; ++i)
  {
    const float value = generator.unit();
    TTS_EXPECT(value >= 0.0f);
    TTS_EXPECT(value < 1.0f);
  }
};

TTS_CASE("A seed of zero is not a degenerate stream")
{
  arpg::rng generator(0);

  int non_zero = 0;
  for (int i = 0; i < 8; ++i)
  {
    non_zero += (generator.next() != 0) ? 1 : 0;
  }

  TTS_EQUAL(non_zero, 8);
};
