// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/vec2.hpp>
#include <data/script_host.hpp>
#include <world/level_layout.hpp>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace arpg
{

/// What a place is made of.
///
/// Every distance here is a multiple of the view rather than a count of
/// pixels: a file saying a room is a screen and a half wide reads better than
/// one saying 480, and stays right when the internal resolution changes.
struct biome
{
  /// How the rest of the data names it, taken from the file name so that two
  /// biomes cannot claim the same one.
  std::string key;

  /// What the player is shown. Translated, so never used to look anything up.
  std::string name;

  level_shape shape = level_shape::organic;

  int rooms_min = 5;
  int rooms_max = 7;

  vec2 room_min{0.75f, 0.75f};
  vec2 room_max{1.25f, 1.25f};

  /// Ground left between two rooms, as a multiple of the view width.
  float spacing = 0.3f;

  /// One kind that lives here, and how often it is offered when a wave is
  /// composed. Separate from what it costs: a brute costing eight parasites is
  /// already rare by the room it fills, but without a weight it would still be
  /// offered as often as one.
  struct dweller
  {
    std::string name;
    int weight = 1;
  };

  /// Written as an ordered list rather than as a table keyed by name: the
  /// iteration order of a Lua table is unspecified, and a world drawn from a
  /// seed cannot depend on it.
  std::vector<dweller> fauna;
};

/// The biomes that were read, or the reason they were not.
struct loaded_biomes
{
  std::vector<biome> all;

  /// Empty when every file made sense. One bad file throws all of them away:
  /// a level fielded from half a catalogue is a strange place rather than a
  /// message.
  std::string error;

  bool valid() const { return error.empty(); }

  /// The biome answering to @p key, or nothing.
  const biome* find(std::string_view key) const;
};

/// Reads one biome from Lua source, named @p key.
loaded_biomes load_biome(script_host& host, std::string_view source, std::string_view key);

/// Reads every `.lua` file in @p directory, in the order of their names.
///
/// Sorted rather than left to the filesystem: the order decides nothing today,
/// and the day it decides something, an unordered walk would make two machines
/// disagree about a level drawn from the same seed.
loaded_biomes load_biomes_from(script_host& host, const std::filesystem::path& directory);

/// Turns a biome into what the generator takes, against a view of @p screen.
level_recipe recipe_for(const biome& place, vec2 screen);

} // namespace arpg
