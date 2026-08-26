// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <data/script_host.hpp>
#include <ecs/enemy.hpp>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace arpg
{

/// What a data file describes: the archetypes, and the names they answer to.
///
/// Rooms and firing patterns will name the enemies they field rather than
/// repeat their figures, which is what the names are kept for.
/// What a file says an archetype looks like, before anything has been loaded.
///
/// Names live here and not in the archetype, which has to stay something a
/// wall of entities can hold a copy of.
struct enemy_look
{
  /// The atlas, under assets/textures. Empty when the file says nothing.
  std::string atlas;

  /// One animation per state, in the order the states are declared.
  std::vector<std::string> clips;
};

struct enemy_catalogue
{
  std::vector<enemy_archetype> kinds;
  std::vector<std::string> names;

  /// One per kind, in the same order.
  std::vector<enemy_look> looks;

  /// Empty when the load succeeded. A catalogue carrying an error holds
  /// nothing: half a roster would field a wave nobody designed, and the
  /// mistake would show up as a strange fight rather than as a message.
  std::string error;

  bool valid() const { return error.empty(); }
};

/// Reads a catalogue from Lua source.
///
/// @p minimum_sight is how far the player can strike. An archetype waking
/// closer than that can be killed without ever noticing, so the load refuses
/// it rather than let it reach a room.
enemy_catalogue load_enemies(script_host& host, std::string_view source, float minimum_sight);

/// Same, reading @p path. A file that cannot be opened is an error like any
/// other rather than an exception.
enemy_catalogue load_enemies_from(script_host& host, const std::filesystem::path& path, float minimum_sight);

} // namespace arpg
