// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstdint>

namespace arpg
{

/// Deterministic pseudo-random generator.
///
/// Written out rather than taken from the standard library on purpose. The
/// engines there are specified, but the distributions are not: the same seed
/// through std::uniform_int_distribution gives different numbers on different
/// implementations, which would break shared seeds, daily runs and reproducing
/// a bug from a seed alone.
///
/// Content that must replay identically draws from this. Anything cosmetic,
/// where nobody can tell the difference, is free to use whatever it likes.
class rng
{
public:
  /// @param seed the run seed. Any value works, including zero.
  explicit rng(std::uint64_t seed);

  /// Next raw value, spanning the whole 32-bit range.
  std::uint32_t next();

  /// Uniform value in [0, @p bound).
  ///
  /// Rejects the tail rather than taking a modulo of the raw value, which would
  /// favour the low end whenever the bound does not divide 2^32.
  ///
  /// @return 0 when @p bound is 0, rather than dividing by it.
  std::uint32_t below(std::uint32_t bound);

  /// Uniform value in [0, 1).
  float unit();

private:
  std::uint64_t m_state = 0;
  std::uint64_t m_step = 0;
};

} // namespace arpg
