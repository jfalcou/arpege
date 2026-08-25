// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <data/script_host.hpp>
#include <ecs/dash.hpp>

#include <filesystem>
#include <string>
#include <string_view>

namespace arpg
{

/// Everything the player is worth, in figures a data file states.
struct player_profile
{
  int health = 3;

  float speed = 70.0f;

  /// Held to trade speed for precision, which is where a hitbox of a few
  /// pixels earns its keep.
  float focus_speed = 30.0f;

  /// Far smaller than the sprite: a wall of bullets must be threadable where
  /// it looks solid.
  float hitbox = 2.0f;

  /// Seconds of invulnerability a hit grants, so losing a third of the life
  /// bar is not immediately followed by losing another.
  float mercy = 0.8f;

  float fire_interval = 0.12f;
  float bullet_speed = 220.0f;
  float bullet_radius = 1.5f;
  float bullet_life = 1.2f;
  int bullet_damage = 1;

  dash_profile dash{};

  /// How far a shot carries before it gives up. The room is wider than the
  /// screen, and nothing the player can reach may still be asleep, so this is
  /// the figure the waking distance of an enemy is checked against.
  float range() const { return bullet_speed * bullet_life; }
};

/// A profile that was read, or the reason it was not.
struct loaded_player
{
  player_profile value;

  /// Empty when the load succeeded. A profile carrying an error is the default
  /// one, never a half-read file: a player with a speed and no health would be
  /// a stranger bug than a message.
  std::string error;

  bool valid() const { return error.empty(); }
};

loaded_player load_player(script_host& host, std::string_view source);
loaded_player load_player_from(script_host& host, const std::filesystem::path& path);

} // namespace arpg
