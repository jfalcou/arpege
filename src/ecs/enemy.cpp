// SPDX-License-Identifier: BSL-1.0

#include <ecs/enemy.hpp>

#include <algorithm>

namespace arpg
{

namespace
{

/// Points per pixel of floor. Tuned so a small room lands around thirty points
/// and a large arena around a hundred and fifty.
constexpr float points_per_area = 0.0026f;

/// Added share per strate, so the same room is harsher further down.
constexpr float depth_bonus = 0.25f;

} // namespace

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
