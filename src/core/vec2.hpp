// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cmath>

namespace arpg
{

struct vec2
{
  float x = 0.0f;
  float y = 0.0f;

  friend bool operator==(vec2, vec2) = default;
};

constexpr vec2 operator+(vec2 a, vec2 b)
{
  return vec2{a.x + b.x, a.y + b.y};
}

constexpr vec2 operator-(vec2 a, vec2 b)
{
  return vec2{a.x - b.x, a.y - b.y};
}

constexpr vec2 operator*(vec2 v, float factor)
{
  return vec2{v.x * factor, v.y * factor};
}

constexpr vec2 operator*(float factor, vec2 v)
{
  return v * factor;
}

constexpr float length_squared(vec2 v)
{
  return v.x * v.x + v.y * v.y;
}

inline float length(vec2 v)
{
  return std::sqrt(length_squared(v));
}

// Returns the zero vector when v is zero, so callers never divide by zero.
inline vec2 normalized(vec2 v)
{
  const float size = length(v);
  return (size > 0.0f) ? vec2{v.x / size, v.y / size} : vec2{};
}

} // namespace arpg
