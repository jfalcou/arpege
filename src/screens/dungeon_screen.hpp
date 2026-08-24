// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/action_map.hpp>
#include <core/action_state.hpp>
#include <core/camera.hpp>
#include <core/rng.hpp>
#include <core/screen.hpp>
#include <data/enemy_data.hpp>
#include <ecs/encounter.hpp>
#include <ecs/enemy.hpp>
#include <ecs/spatial_hash.hpp>

#include <entt/entity/registry.hpp>

#include <vector>

namespace arpg
{

/// A room of the dungeon: the player, what shoots at them, and what they shoot.
///
/// Owns its world outright. A screen underneath, the strate map or a pause,
/// keeps its own and is untouched by what happens here.
class dungeon_screen : public screen
{
public:
  void on_enter() override;
  void update(float dt) override;
  void render(float alpha) override;

private:
  /// The room, larger than the screen: the camera follows the player across it.
  viewport_rect room() const;

  /// What is on screen, in room coordinates.
  vec2 view() const;

  void spawn_player();
  void spawn_wave();
  /// Whether the player is still standing. Their entity is destroyed on death
  /// like any other, so everything reaching for it has to ask first.
  bool player_alive() const;

  /// Wipes the room from the debug key, sparing the wait to reach its end.
  void purge_enemies();

  /// Opens the way out and announces the room, once the last enemy falls.
  void settle_room();

  void steer_player();
  void fire(float dt);

  entt::registry m_world;
  entt::entity m_player = entt::null;

  action_map m_bindings;
  action_state m_actions;

  /// Cells are as wide as the largest collider in play, which is what the grid
  /// expects to be able to widen a query by a single cell.
  spatial_hash m_hash{16.0f};

  /// Reused by the collision pass so querying does not allocate every step.
  std::vector<entt::entity> m_scratch;

  float m_fire_cooldown = 0.0f;

  /// Outlives every load, since the tables the scripts return borrow it.
  script_host m_scripts;

  /// Read from assets at every entry, so a change to the file shows up on the
  /// next room rather than on the next build.
  enemy_catalogue m_roster;

  encounter m_fight;
  exit_portal m_exit;

  /// Counted per simulation step, so the enemies can take turns thinking.
  std::uint64_t m_step = 0;

  /// Seeded per room. Everything that must replay identically draws from here.
  rng m_generator{1};

  camera_focus m_camera;
};

} // namespace arpg
