// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/action_map.hpp>
#include <core/action_state.hpp>
#include <core/camera.hpp>
#include <core/rng.hpp>
#include <core/screen.hpp>
#include <core/sprite_store.hpp>
#include <data/biome_data.hpp>
#include <data/enemy_data.hpp>
#include <data/hot_reload.hpp>
#include <data/player_data.hpp>
#include <ecs/encounter.hpp>
#include <ecs/enemy.hpp>
#include <ecs/spatial_hash.hpp>
#include <world/level_run.hpp>
#include <world/run_state.hpp>

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
  /// Takes the posting by reference and changes it: what a level costs and
  /// what it yields has to outlive the level.
  explicit dungeon_screen(run_state& run)
    : m_run(&run)
  {
  }

  void on_enter() override;
  void update(float dt) override;
  void render(float alpha) override;

private:
  /// How the level is laid out, which the biome in play names.
  level_recipe level_shape_in_use() const;

  /// Draws which biome this level is in, and gathers what lives there.
  void choose_biome();

  /// The room the player stands in, in level coordinates.
  viewport_rect room() const;

  /// Finds @p name among the sheets in play, loading it if it is not there
  /// yet. Answers where it sits, and whether it could be had at all.
  bool sheet_index_of(const std::string& name, std::uint16_t& into);

  /// Turns the names a roster states into the handles the world carries.
  void dress_roster();

  /// Draws @p who at @p at from its sheet. False when it has no picture, and
  /// the caller falls back on the shape that stood in for one.
  bool draw_sprite(entt::entity who, vec2 at);

  /// A plan of the level in a corner, since a level larger than the screen
  /// cannot be read from inside one of its rooms.
  void draw_minimap();

  /// Asks to leave the level, once and once only.
  void leave();

  /// Wipes the world and rebuilds it around the room the run is now in.
  void enter_current_room();

  /// Walks through a door once the room is clear and the player reaches one.
  void take_doors();

  /// What is on screen, in room coordinates.
  vec2 view() const;

  void spawn_player(vec2 at);
  void spawn_wave();
  /// Whether the player is still standing. Their entity is destroyed on death
  /// like any other, so everything reaching for it has to ask first.
  bool player_alive() const;

  /// Wipes the room from the debug key, sparing the wait to reach its end.
  void purge_enemies();

  /// Reads both data files, leaving what is in place untouched if either is
  /// refused. False when nothing was replaced.
  bool read_content();

  /// Re-reads the data files and re-forms the room from the same seed.
  void reload_content();

  /// Opens the way out and announces the room, once the last enemy falls.
  void settle_room();

  void steer_player(float dt);
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

  /// Same, for the headings a volley leaves along.
  std::vector<vec2> m_headings;

  float m_fire_cooldown = 0.0f;

  dash_state m_dash;

  /// Outlives every load, since the tables the scripts return borrow it.
  script_host m_scripts;

  /// Where the pictures come from. Built at entry, since it needs the asset
  /// root, which a screen only has once it has been handed its services.
  std::optional<sprite_store> m_sprites;

  /// The sheets in play, which an appearance indexes into, and what they are
  /// called, so a second archetype asking for one already loaded finds it.
  std::vector<sheet> m_sheets;
  std::vector<std::string> m_sheet_names;

  /// Read from assets at every entry, and re-read whenever a file changes.
  player_profile m_profile;
  enemy_catalogue m_roster;
  loaded_biomes m_biomes;

  /// The place this level is in, and the part of the roster that lives there.
  biome m_biome;
  std::vector<enemy_archetype> m_fauna;
  std::vector<int> m_weights;

  file_watch m_player_watch;
  file_watch m_roster_watch;
  directory_watch m_biomes_watch;

  /// Why the last read failed, shown in place rather than left to a log
  /// nobody has open while tuning.
  std::string m_data_error;

  level_run m_level;

  /// Which room was left to get here, so the player is set down at the door
  /// they came through. Out of range while nowhere has been left yet.
  std::size_t m_came_from = static_cast<std::size_t>(-1);

  /// Carried from room to room, since the player entity is rebuilt with the
  /// world every time one is entered.
  int m_carried_health = 0;

  encounter m_fight;
  exit_portal m_exit;

  /// Counted per simulation step, so the enemies can take turns thinking.
  std::uint64_t m_step = 0;

  /// The posting this level belongs to, owned by the Bureau below.
  run_state* m_run = nullptr;

  /// Set once the screen has asked to leave. A pop is applied at the end of a
  /// frame, and a frame holds up to five simulation steps: without this, a
  /// screen keeps running after asking to go and asks again, popping whatever
  /// was underneath it as well.
  bool m_leaving = false;

  /// Seeded per level, derived from the posting rather than drawn here.
  std::uint64_t m_seed = 1;
  rng m_generator{m_seed};

  camera_focus m_camera;
};

} // namespace arpg
