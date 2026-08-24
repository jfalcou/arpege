// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/action_map.hpp>
#include <core/action_state.hpp>
#include <core/camera.hpp>
#include <core/rng.hpp>
#include <core/screen.hpp>
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

  /// How many enemies are still standing.
  std::size_t enemies_left() const;

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

  /// How many the room opened with, so what is left can be counted against it.
  std::size_t m_enemies_at_start = 0;

  /// Counted per simulation step, so the enemies can take turns thinking.
  std::uint64_t m_step = 0;

  /// Seeded per room. Everything that must replay identically draws from here.
  rng m_generator{1};

  camera_focus m_camera;
};

} // namespace arpg
