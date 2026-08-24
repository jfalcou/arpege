// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cmath>

/// Everything the engine defines.
namespace arpg
{

/// A two dimensional vector, used for positions, directions and sizes alike.
///
/// Deliberately independent from the vector type of the rendering backend, so
/// that the logic using it can be built and tested without linking one.
struct vec2
{
  float x = 0.0f; ///< Rightwards.
  float y = 0.0f; ///< Downwards, as screen coordinates go.

  /// Exact comparison, floating point caveats included.
  friend bool operator==(vec2, vec2) = default;
};

/// @name Vector arithmetic
/// @{

/// Component-wise sum.
constexpr vec2 operator+(vec2 a, vec2 b)
{
  return vec2{a.x + b.x, a.y + b.y};
}

/// Component-wise difference.
constexpr vec2 operator-(vec2 a, vec2 b)
{
  return vec2{a.x - b.x, a.y - b.y};
}

/// Scales @p v.
constexpr vec2 operator*(vec2 v, float factor)
{
  return vec2{v.x * factor, v.y * factor};
}

/// Scales @p v.
constexpr vec2 operator*(float factor, vec2 v)
{
  return v * factor;
}

/// @}

/// Squared magnitude, when comparing lengths is enough and the square root is
/// not worth paying for.
constexpr float length_squared(vec2 v)
{
  return v.x * v.x + v.y * v.y;
}

/// Magnitude of @p v.
inline float length(vec2 v)
{
  return std::sqrt(length_squared(v));
}

/// Unit vector pointing the same way as @p v.
///
/// @return the zero vector when @p v is zero, so callers never divide by zero.
inline vec2 normalized(vec2 v)
{
  const float size = length(v);
  return (size > 0.0f) ? vec2{v.x / size, v.y / size} : vec2{};
}

} // namespace arpg
