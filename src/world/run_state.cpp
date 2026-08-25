// SPDX-License-Identifier: BSL-1.0

#include <world/run_state.hpp>

#include <algorithm>

namespace arpg
{

namespace
{

/// Mixes a posting seed with a depth into a seed of its own.
///
/// The odd constants are the usual splitmix finaliser: neighbouring depths
/// must not give neighbouring seeds, or two levels of the same posting would
/// come out looking like each other.
std::uint64_t mix(std::uint64_t value)
{
  value += 0x9E3779B97F4A7C15ull;
  value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ull;
  value = (value ^ (value >> 27)) * 0x94D049BB133111EBull;

  return value ^ (value >> 31);
}

} // namespace

run_state begin_posting(std::uint64_t seed, int health)
{
  run_state run;
  run.seed = seed;
  run.health_max = std::max(1, health);
  run.health = run.health_max;

  return run;
}

bool employee_alive(const run_state& run)
{
  return run.health > 0;
}

std::uint64_t level_seed(const run_state& run, int depth)
{
  return mix(run.seed + mix(static_cast<std::uint64_t>(depth)));
}

std::uint64_t current_level_seed(const run_state& run)
{
  return level_seed(run, run.depth);
}

void finish_level(run_state& run)
{
  ++run.depth;
}

void bank_loot(run_state& run)
{
  run.banked += run.carried;
  run.carried = 0;
}

void lose_employee(run_state& run)
{
  run.carried = 0;
  run.health = 0;
  ++run.lost;
}

void assign_employee(run_state& run, int health)
{
  run.health_max = std::max(1, health);
  run.health = run.health_max;
  run.carried = 0;
}

} // namespace arpg
