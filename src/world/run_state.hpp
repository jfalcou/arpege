// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstdint>

namespace arpg
{

/// What a posting is worth so far.
///
/// This is the only thing that survives a visit to the Failles. It holds no
/// entity and no screen: the dungeon takes it, changes it, and hands it back,
/// which is what makes a run something other than a level played twice.
struct run_state
{
  /// Drawn when the posting is signed. Every level of it derives from this
  /// rather than drawing one of its own, so a posting resumed from a save
  /// lays out the same places.
  std::uint64_t seed = 0;

  int health = 3;
  int health_max = 3;

  /// Sent back to the Bureau and safe. Nothing takes this away.
  int banked = 0;

  /// Found and still being carried. Worth more, owed to nobody, and lost with
  /// whoever was carrying it.
  int carried = 0;

  /// How many levels of this posting have been finished.
  int depth = 0;

  /// How many employees it has cost so far.
  int lost = 0;
};

/// Signs a posting: full health, nothing carried, nothing owed.
run_state begin_posting(std::uint64_t seed, int health);

/// Whether whoever holds the posting is still standing.
bool employee_alive(const run_state& run);

/// The seed a level is laid out from.
///
/// Derived from the posting rather than drawn and stored: a save that kept
/// only the posting seed must still give back the same level, and a seed
/// shared between two players must give them the same one.
std::uint64_t level_seed(const run_state& run, int depth);

/// Same, for the level being played now.
std::uint64_t current_level_seed(const run_state& run);

/// Records a level finished, which is what moves the posting on.
void finish_level(run_state& run);

/// Sends what is carried back to the Bureau, where nothing can take it.
void bank_loot(run_state& run);

/// Buries the employee. What they carried goes with them; the posting stands,
/// and so does what was already sent back.
void lose_employee(run_state& run);

/// Puts a new employee on the posting, at full health.
void assign_employee(run_state& run, int health);

} // namespace arpg
