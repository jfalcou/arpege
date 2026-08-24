// SPDX-License-Identifier: BSL-1.0

#include <core/rng.hpp>

namespace arpg
{

namespace
{

/// Multiplier of the LCG underlying the generator, from the reference
/// implementation of PCG.
constexpr std::uint64_t multiplier = 6364136223846793005ULL;

/// Any odd number works as the increment; this one keeps a seed of zero from
/// producing a degenerate stream.
constexpr std::uint64_t default_increment = 1442695040888963407ULL;

} // namespace

rng::rng(std::uint64_t seed)
  : m_state(seed + default_increment)
  , m_step(default_increment)
{
  // One step so that neighbouring seeds do not start on neighbouring values.
  next();
}

std::uint32_t rng::next()
{
  const std::uint64_t previous = m_state;
  m_state = previous * multiplier + m_step;

  // The output is a permutation of the state rather than the state itself: an
  // LCG on its own has weak low bits, which is exactly what below() uses.
  const auto shifted = static_cast<std::uint32_t>(((previous >> 18U) ^ previous) >> 27U);
  const auto rotation = static_cast<unsigned int>(previous >> 59U);

  return (shifted >> rotation) | (shifted << ((32U - rotation) & 31U));
}

std::uint32_t rng::below(std::uint32_t bound)
{
  if (bound == 0)
  {
    return 0;
  }

  // Values under the threshold would make the range uneven, so they are drawn
  // again. The loop is expected to run once.
  const std::uint32_t threshold = (0U - bound) % bound;

  while (true)
  {
    const std::uint32_t value = next();

    if (value >= threshold)
    {
      return value % bound;
    }
  }
}

float rng::unit()
{
  // Divided by 2^32 so the result never reaches 1.
  return static_cast<float>(next()) * 2.3283064e-10f;
}

} // namespace arpg
