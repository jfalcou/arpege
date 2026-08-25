// SPDX-License-Identifier: BSL-1.0

#include <ecs/enemy.hpp>

#include <algorithm>

namespace arpg
{

namespace
{

/// Points per pixel of floor.
///
/// Tuned against a room of two screens by two, which is what the camera makes
/// of one: around thirteen enemies, four of them shooters. Three times this
/// came from back when a room was a single screen and fielded thirty-four;
/// half of it left the room too quiet to be a fight.
constexpr float points_per_area = 0.00087f;

/// Added share per strate, so the same room is harsher further down.
constexpr float depth_bonus = 0.25f;

} // namespace

vec2 pick_spawn(rng& generator, viewport_rect room, float radius, std::span<const vec2> keep_clear, float clearance)
{
  // How many spots are offered before one is taken anyway.
  constexpr int attempts = 20;

  const float span_x = std::max(0.0f, room.width - 2.0f * radius);
  const float span_y = std::max(0.0f, room.height - 2.0f * radius);

  vec2 spot{};

  for (int attempt = 0; attempt < attempts; ++attempt)
  {
    spot = vec2{room.x + radius + generator.unit() * span_x, room.y + radius + generator.unit() * span_y};

    const bool crowded = std::any_of(keep_clear.begin(), keep_clear.end(),
                                     [&](vec2 avoid) { return length_squared(spot - avoid) < clearance * clearance; });

    if (!crowded)
    {
      break;
    }
  }

  return spot;
}

int combat_budget(float area, int depth)
{
  if (area <= 0.0f || depth <= 0)
  {
    return 0;
  }

  const float scaled = area * points_per_area * (1.0f + depth_bonus * static_cast<float>(depth - 1));
  return static_cast<int>(scaled);
}

std::vector<std::size_t> compose_wave(int budget, std::span<const enemy_archetype> catalogue, rng& generator)
{
  std::vector<std::size_t> picked;

  if (catalogue.empty() || budget <= 0)
  {
    return picked;
  }

  const auto cheapest =
      std::min_element(catalogue.begin(), catalogue.end(),
                       [](const enemy_archetype& a, const enemy_archetype& b) { return a.cost < b.cost; });

  // A free archetype would compose an endless wave; refusing the whole
  // catalogue is better than filling a room with an infinity of them.
  if (cheapest->cost <= 0)
  {
    return picked;
  }

  int left = budget;

  while (left >= cheapest->cost)
  {
    // Only what is still affordable is considered, so the draw never has to be
    // thrown away and the loop always moves forward.
    std::vector<std::size_t> affordable;

    for (std::size_t i = 0; i < catalogue.size(); ++i)
    {
      if (catalogue[i].cost > 0 && catalogue[i].cost <= left)
      {
        affordable.push_back(i);
      }
    }

    const std::size_t chosen = affordable[generator.below(static_cast<std::uint32_t>(affordable.size()))];
    picked.push_back(chosen);
    left -= catalogue[chosen].cost;
  }

  return picked;
}

} // namespace arpg
