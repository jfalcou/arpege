// SPDX-License-Identifier: BSL-1.0

#include <screens/dungeon_screen.hpp>

#include <core/application.hpp>
#include <core/events.hpp>
#include <core/pixel_canvas.hpp>
#include <core/raylib_input.hpp>
#include <core/screen_manager.hpp>
#include <ecs/systems.hpp>
#include <world/level_layout.hpp>

#include <raylib.h>

#include <algorithm>
#include <array>

namespace arpg
{

namespace
{

/// Where the data files are read from, under the asset root. Biomes are a
/// directory rather than a file, so that a mod adds one by dropping a file in.
constexpr const char* player_file = "data/player.lua";
constexpr const char* roster_file = "data/enemies.lua";
constexpr const char* biomes_directory = "data/biomes";

/// How close the player must come to a door for it to take them.
constexpr float door_radius = 22.0f;

/// What the boss room is worth against an ordinary one, as a depth for the
/// combat budget.
constexpr int boss_depth = 4;

/// How much ground is left clear around where the player sets foot and around
/// every doorway, since either is somewhere they may appear.
constexpr float spawn_clearance = 72.0f;

/// How eagerly the view catches up, in units per second. Tight enough to feel
/// attached, loose enough not to judder on a fixed step.
constexpr float camera_stiffness = 8.0f;

Vector2 to_raylib(vec2 value)
{
  return Vector2{value.x, value.y};
}

vec2 interpolated(const transform& place, float alpha)
{
  return place.previous + (place.position - place.previous) * alpha;
}

} // namespace

void dungeon_screen::on_enter()
{
  m_bindings = dungeon_bindings(raylib_input::codes());

  // The player's own range is what the roster is checked against: an enemy
  // waking closer than that could never answer.
  m_player_watch = file_watch{*ctx().assets / player_file};
  m_roster_watch = file_watch{*ctx().assets / roster_file};
  m_biomes_watch = directory_watch{*ctx().assets / biomes_directory};

  read_content();
  choose_biome();

  m_level = begin_level(generate_level(level_shape_in_use(), m_generator));
  m_carried_health = m_profile.health;

  enter_current_room();
}

level_recipe dungeon_screen::level_shape_in_use() const
{
  // Every distance a biome states is a multiple of the view, so a room keeps
  // the shape of a screen whatever the canvas is.
  return recipe_for(m_biome, view());
}

viewport_rect dungeon_screen::room() const
{
  if (m_level.here >= m_level.layout.rooms.size())
  {
    const vec2 screen = view();
    return viewport_rect{0.0f, 0.0f, screen.x, screen.y};
  }

  return m_level.layout.rooms[m_level.here].bounds;
}

vec2 dungeon_screen::view() const
{
  const pixel_canvas& canvas = *ctx().canvas;
  return vec2{static_cast<float>(canvas.width()), static_cast<float>(canvas.height())};
}

void dungeon_screen::enter_current_room()
{
  // A level with no room in it means the data was refused. Reading a role out
  // of an empty layout would be the first thing to go wrong, and the least
  // legible.
  if (m_level.here >= m_level.layout.rooms.size())
  {
    m_world.clear();
    m_player = entt::null;
    return;
  }

  // Taken before the world goes, since the entity holding it goes with it.
  // Without this a room change would hand the player a full life bar, which
  // is a stranger bug than a crash for being invisible.
  if (m_world.valid(m_player) && m_world.all_of<health>(m_player))
  {
    m_carried_health = m_world.get<health>(m_player).current;
  }

  m_world.clear();
  m_dash = dash_state{};
  m_fire_cooldown = 0.0f;
  m_fight = encounter{};
  m_exit = exit_portal{};

  const viewport_rect bounds = room();

  // Set down just inside the door led back to, rather than in the middle of
  // the room: without a corridor to walk, this is what keeps crossing a
  // threshold from reading as being teleported.
  vec2 landing{bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f};

  if (m_came_from < m_level.layout.rooms.size() && m_came_from != m_level.here)
  {
    const vec2 back = door_position(bounds, m_level.layout.rooms[m_came_from].bounds);
    const vec2 towards_middle = landing - back;

    // A doorway falling exactly on the middle of the room leaves no direction
    // to step inward along, and normalising nothing gives a position that is
    // not a number: every comparison against it then answers false, which
    // reads as the room having no walls and no doors.
    if (length_squared(towards_middle) > 0.0f)
    {
      landing = back + normalized(towards_middle) * (door_radius + 12.0f);
    }
  }

  spawn_player(landing);

  // A landing and a service room hold nothing: arriving in a fight one has not
  // seen coming reads as an ambush rather than as a level.
  const room_role role = m_level.layout.rooms[m_level.here].role;
  const bool peaceful = role == room_role::start || role == room_role::station;

  if (room_is_clear(m_level) || peaceful)
  {
    clear_room(m_level);
    m_fight = encounter{};
    settle_room();
  }
  else
  {
    spawn_wave();
  }
}

void dungeon_screen::take_doors()
{
  const viewport_rect here = room();
  const vec2 player_at = m_world.get<transform>(m_player).position;

  for (const std::size_t neighbour : open_doors(m_level))
  {
    const vec2 door = door_position(here, m_level.layout.rooms[neighbour].bounds);

    if (length_squared(player_at - door) > door_radius * door_radius)
    {
      continue;
    }

    const std::size_t leaving = m_level.here;

    if (enter_room(m_level, neighbour))
    {
      m_came_from = leaving;
      enter_current_room();
      return;
    }
  }
}

void dungeon_screen::spawn_player(vec2 at)
{
  const vec2 middle = at;

  m_player = m_world.create();
  m_world.emplace<transform>(m_player, middle, middle);
  m_world.emplace<velocity>(m_player);
  m_world.emplace<collider>(m_player, m_profile.hitbox);
  m_world.emplace<team>(m_player, faction::player);
  m_world.emplace<health>(m_player, std::max(1, m_carried_health), m_profile.health);
  m_world.emplace<invulnerable>(m_player, 0.0f, m_profile.mercy);
  m_world.emplace<confined>(m_player);
  m_world.emplace<player_controlled>(m_player);

  // Snapped rather than eased on the first step, or the room would slide into
  // place from a corner every time it opens.
  m_camera.centre = follow_camera(middle, middle, room(), view(), 0.0f, 0.0f);
}

void dungeon_screen::spawn_wave()
{
  if (m_fauna.empty())
  {
    return;
  }

  const viewport_rect bounds = room();
  const float width = bounds.width;
  const float height = bounds.height;

  // The room pays for its own enemies: what fits in the budget is what shows
  // up, so a bigger room is dangerous in proportion rather than by luck. The
  // boss room counts as deeper, which is what makes it the end of the level
  // while there is no boss to put in it.
  const int depth = (m_level.layout.rooms[m_level.here].role == room_role::boss) ? boss_depth : 1;
  const int budget = combat_budget(width * height, depth);
  const auto composition = compose_wave(budget, m_fauna, m_weights, m_generator);

  // Every doorway of this room, since the player may come back through any of
  // them and must not walk into a body on the threshold.
  std::vector<vec2> doorways;

  for (const std::size_t neighbour : neighbours_of(m_level.layout, m_level.here))
  {
    doorways.push_back(door_position(bounds, m_level.layout.rooms[neighbour].bounds));
  }

  doorways.push_back(m_world.get<transform>(m_player).position);

  std::uint8_t slice = 0;

  for (const std::size_t index : composition)
  {
    const enemy_archetype& kind = m_fauna[index];

    const vec2 spot = pick_spawn(m_generator, bounds, kind.radius, doorways, spawn_clearance);

    const entt::entity foe = m_world.create();
    m_world.emplace<transform>(foe, spot, spot);
    m_world.emplace<velocity>(foe);
    m_world.emplace<collider>(foe, kind.radius);
    m_world.emplace<team>(foe, faction::enemy);
    m_world.emplace<health>(foe, kind.health, kind.health);
    m_world.emplace<enemy_archetype>(foe, kind);
    m_world.emplace<damage>(foe, kind.touch);

    // Staggered, so a wave that spawns together does not fire in one volley.
    m_world.emplace<weapon>(foe, m_generator.unit() * kind.shots.interval);
    m_world.emplace<confined>(foe);

    // Dealt round-robin so the crowd is spread evenly over the thinking
    // rounds instead of everyone landing in the same one.
    m_world.emplace<enemy_brain>(foe, enemy_state::idle, 0.0f, slice);
    slice = static_cast<std::uint8_t>((slice + 1) % 4);
  }

  m_fight.opened_with = composition.size();
}

void dungeon_screen::draw_minimap()
{
  if (m_level.layout.rooms.empty())
  {
    return;
  }

  // A level larger than the screen cannot be read from inside one of its
  // rooms: without a plan of it, choosing a door is choosing blind.
  float min_x = m_level.layout.rooms[0].bounds.x;
  float min_y = m_level.layout.rooms[0].bounds.y;
  float max_x = min_x;
  float max_y = min_y;

  for (const level_room& each : m_level.layout.rooms)
  {
    min_x = std::min(min_x, each.bounds.x);
    min_y = std::min(min_y, each.bounds.y);
    max_x = std::max(max_x, each.bounds.x + each.bounds.width);
    max_y = std::max(max_y, each.bounds.y + each.bounds.height);
  }

  constexpr float plan_width = 54.0f;
  constexpr float plan_height = 34.0f;

  const float left = static_cast<float>(ctx().canvas->width()) - plan_width - 4.0f;
  const float top = static_cast<float>(ctx().canvas->height()) - plan_height - 4.0f;

  // One scale for both axes, or a level wider than it is tall would come out
  // square and the plan would lie about where things are.
  const float scale = std::min(plan_width / (max_x - min_x), plan_height / (max_y - min_y));

  DrawRectangle(static_cast<int>(left) - 2, static_cast<int>(top) - 2, static_cast<int>(plan_width) + 4,
                static_cast<int>(plan_height) + 4, Color{12, 10, 16, 200});

  // The links first, under the rooms. Nothing joins two rooms in the world —
  // a door is a threshold, not a corridor — so the plan is the only place the
  // shape of a level can be read at all.
  for (const level_link& link : m_level.layout.links)
  {
    const viewport_rect& a = m_level.layout.rooms[link.from].bounds;
    const viewport_rect& b = m_level.layout.rooms[link.to].bounds;

    DrawLineV(Vector2{left + (a.x + a.width * 0.5f - min_x) * scale, top + (a.y + a.height * 0.5f - min_y) * scale},
              Vector2{left + (b.x + b.width * 0.5f - min_x) * scale, top + (b.y + b.height * 0.5f - min_y) * scale},
              Color{120, 110, 130, 255});
  }

  for (std::size_t index = 0; index < m_level.layout.rooms.size(); ++index)
  {
    const viewport_rect& each = m_level.layout.rooms[index].bounds;

    const int x = static_cast<int>(left + (each.x - min_x) * scale);
    const int y = static_cast<int>(top + (each.y - min_y) * scale);
    const int w = std::max(2, static_cast<int>(each.width * scale));
    const int h = std::max(2, static_cast<int>(each.height * scale));

    const bool done = index < m_level.cleared.size() && m_level.cleared[index];

    Color tint = done ? Color{92, 84, 104, 255} : Color{44, 38, 54, 255};

    if (m_level.layout.rooms[index].role == room_role::boss)
    {
      tint = done ? Color{188, 84, 84, 255} : Color{96, 44, 44, 255};
    }
    else if (m_level.layout.rooms[index].role == room_role::station)
    {
      tint = Color{72, 112, 84, 255};
    }

    if (index == m_level.here)
    {
      tint = Color{226, 205, 154, 255};
    }

    DrawRectangle(x, y, w, h, tint);
  }
}

void dungeon_screen::purge_enemies()
{
  m_scratch.clear();

  // Their shots go with them, or a bullet fired by something already dead
  // could still kill the player on the way to the rift.
  for (auto [entity, side] : m_world.view<const team>().each())
  {
    if (side.side == faction::enemy)
    {
      m_scratch.push_back(entity);
    }
  }

  // Gathered first: destroying entities while walking the view they come from
  // pulls the ground from under the iteration.
  m_world.destroy(m_scratch.begin(), m_scratch.end());
}

bool dungeon_screen::read_content()
{
  const loaded_player hero = load_player_from(m_scripts, m_player_watch.path());

  if (!hero.valid())
  {
    m_data_error = hero.error;
    return false;
  }

  // Checked against what the player can reach, which the file just gave us:
  // the two data files answer to each other rather than to a constant.
  enemy_catalogue roster = load_enemies_from(m_scripts, m_roster_watch.path(), hero.value.range());

  if (!roster.valid())
  {
    m_data_error = roster.error;
    return false;
  }

  loaded_biomes places = load_biomes_from(m_scripts, *ctx().assets / biomes_directory);

  if (!places.valid())
  {
    m_data_error = places.error;
    return false;
  }

  // A biome fields its fauna by name, and the roster is the only place those
  // names exist. Left unchecked, a typo would quietly give a place one enemy
  // fewer than it was written with.
  for (const biome& place : places.all)
  {
    for (const biome::dweller& lives_here : place.fauna)
    {
      if (std::find(roster.names.begin(), roster.names.end(), lives_here.name) == roster.names.end())
      {
        m_data_error = place.key + ": nothing in the roster is called '" + lives_here.name + "'";
        return false;
      }
    }
  }

  m_profile = hero.value;
  m_roster = std::move(roster);
  m_biomes = std::move(places);
  m_data_error.clear();

  return true;
}

void dungeon_screen::choose_biome()
{
  if (m_biomes.all.empty())
  {
    return;
  }

  m_biome = m_biomes.all[m_generator.below(static_cast<std::uint32_t>(m_biomes.all.size()))];

  // Gathered once here rather than looked up per wave: what lives in a place
  // does not change while the player is in it.
  m_fauna.clear();
  m_weights.clear();

  for (const biome::dweller& lives_here : m_biome.fauna)
  {
    const auto found = std::find(m_roster.names.begin(), m_roster.names.end(), lives_here.name);

    if (found != m_roster.names.end())
    {
      m_fauna.push_back(m_roster.kinds[static_cast<std::size_t>(std::distance(m_roster.names.begin(), found))]);
      m_weights.push_back(lives_here.weight);
    }
  }
}

void dungeon_screen::reload_content()
{
  // A file caught halfway through an edit must not empty the room: what made
  // sense last stays in place, and the reason is shown.
  if (!read_content())
  {
    return;
  }

  // Re-formed from the same seed, so a changed figure is judged against the
  // room it was changed for rather than against a new one. The level comes out
  // identical for that reason, which is what lets the room the player stands in
  // and what they have already cleared survive the reload.
  const std::size_t here = m_level.here;
  const std::vector<bool> cleared = m_level.cleared;

  m_generator = rng{m_seed};
  choose_biome();
  m_level = begin_level(generate_level(level_shape_in_use(), m_generator));

  if (m_level.cleared.size() == cleared.size())
  {
    m_level.cleared = cleared;
    m_level.here = here;
  }

  m_carried_health = m_profile.health;

  enter_current_room();
}

void dungeon_screen::settle_room()
{
  if (!advance_encounter(m_fight, m_world))
  {
    return;
  }

  const viewport_rect bounds = room();
  m_exit.centre = vec2{bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f};

  ctx().events->trigger(room_cleared{m_fight.opened_with});
}

bool dungeon_screen::player_alive() const
{
  return m_world.valid(m_player);
}

void dungeon_screen::steer_player(float dt)
{
  const action_set held = m_bindings.resolve(*ctx().input);
  const vec2 heading = movement_direction(held, ctx().input->left_stick);

  // Buffered rather than read on the edge: a dash asked for between two steps
  // is still honoured, which is most of what makes one feel responsive.
  if (advance_dash(m_dash, m_profile.dash, m_actions.consume(action::dash), heading, dt))
  {
    // Granted here rather than inside the dash, which knows nothing of the
    // world. Never shortened: a dash taken right after a hit must not cut the
    // mercy that hit bought.
    invulnerable& shield = m_world.get<invulnerable>(m_player);
    shield.remaining = std::max(shield.remaining, m_profile.dash.mercy);
  }

  if (dashing(m_dash))
  {
    // The steering is ignored for the whole dash: committing to a direction is
    // what makes it a decision rather than a faster way to walk.
    m_world.get<velocity>(m_player).value = dash_velocity(m_dash, m_profile.dash);
    return;
  }

  // Focus trades speed for precision, and is where the tiny hitbox earns its
  // keep.
  const float speed = m_actions.held(action::focus) ? m_profile.focus_speed : m_profile.speed;

  m_world.get<velocity>(m_player).value = heading * speed;
}

void dungeon_screen::fire(float dt)
{
  m_fire_cooldown -= dt;

  if (!m_actions.held(action::shoot) || m_fire_cooldown > 0.0f)
  {
    return;
  }

  m_fire_cooldown = m_profile.fire_interval;

  const vec2 from = m_world.get<transform>(m_player).position;
  const aim_input aim = resolve_aim(*ctx().input, ctx().input->device);

  // The mouse is reported in screen pixels while the player stands in the room,
  // so an absolute aim is carried across before anything is subtracted from it.
  // The input layer knows neither where the player is nor where the view looks.
  const vec2 pointed = aim.value + view_origin(m_camera.centre, view());
  vec2 heading = aim.absolute ? normalized(pointed - from) : aim.value;

  if (length_squared(heading) <= 0.0f)
  {
    heading = vec2{0.0f, -1.0f};
  }

  const entt::entity shot = m_world.create();
  m_world.emplace<transform>(shot, from, from);
  m_world.emplace<velocity>(shot, heading * m_profile.bullet_speed);
  m_world.emplace<collider>(shot, m_profile.bullet_radius);
  m_world.emplace<team>(shot, faction::player);
  m_world.emplace<damage>(shot, m_profile.bullet_damage);
  m_world.emplace<lifetime>(shot, m_profile.bullet_life);
  m_world.emplace<projectile>(shot);
}

void dungeon_screen::update(float dt)
{
  m_actions.advance(m_bindings.resolve(*ctx().input));

  // A step that finds no world to play is one whose room could not be built.
  if (m_level.layout.rooms.empty())
  {
    return;
  }

  if (m_actions.consume(action::pause) || !player_alive())
  {
    ctx().screens->pop();
    return;
  }

  // Reaching the end of a room the honest way takes minutes, which is too slow
  // a loop to tune what happens once it is cleared. Read straight from raylib
  // rather than through the action layer: this is a tool, not a move the game
  // knows about.
  if (IsKeyPressed(KEY_F9))
  {
    purge_enemies();
  }

  // Both are polled, since a change to either invalidates the room: the roster
  // is checked against what the player can reach.
  const bool player_changed = m_player_watch.poll(dt);
  const bool roster_changed = m_roster_watch.poll(dt);
  const bool biomes_changed = m_biomes_watch.poll(dt);

  if (player_changed || roster_changed || biomes_changed)
  {
    reload_content();
  }

  steer_player(dt);
  fire(dt);

  const vec2 player_at = m_world.get<transform>(m_player).position;

  advance_brains(m_world, dt, m_step, player_at);
  fire_enemy_weapons(m_world, dt, player_at, m_headings);
  ++m_step;

  integrate_motion(m_world, dt);
  expire_lifetimes(m_world, dt);

  const viewport_rect bounds = room();

  // After the motion that may have pushed someone through an edge, and before
  // the collisions, so nothing is ever resolved against a position outside.
  confine_to_bounds(m_world, bounds);
  despawn_out_of_bounds(m_world, bounds, 16.0f);

  m_camera.centre =
      follow_camera(m_camera.centre, m_world.get<transform>(m_player).position, bounds, view(), dt, camera_stiffness);

  tick_invulnerability(m_world, dt);
  rebuild_spatial_hash(m_world, m_hash);
  resolve_projectile_hits(m_world, m_hash, m_scratch);
  resolve_contact_damage(m_world, m_hash, m_scratch);

  if (!player_alive())
  {
    // Dying destroys the entity like any other, and everything above reads it.
    // Leaving now beats simulating a room around a corpse; a defeat screen
    // takes this place later.
    ctx().screens->pop();
    return;
  }

  settle_room();

  if (m_fight.state == encounter_state::cleared)
  {
    clear_room(m_level);
  }

  if (level_finished(m_level) && enter_portal(m_exit, m_world.get<transform>(m_player).position))
  {
    ctx().screens->pop();
    return;
  }

  take_doors();
}

void dungeon_screen::render(float alpha)
{
  // Outside the room, so the floor has something to be told apart from.
  ClearBackground(Color{6, 5, 9, 255});

  const vec2 origin = view_origin(m_camera.centre, view());
  const viewport_rect bounds = room();

  const Rectangle floor{bounds.x - origin.x, bounds.y - origin.y, bounds.width, bounds.height};

  // Three shades rather than a single outline: the void, the floor standing
  // out from it, and a wall thick enough to read at this resolution. An edge
  // one pixel wide and barely lighter than the background could not be seen.
  DrawRectangleRec(floor, Color{24, 20, 30, 255});
  DrawRectangleLinesEx(floor, 2.0f, Color{86, 72, 102, 255});

  // Every way out of a room that has been emptied. Drawn on the floor so the
  // player walks over them rather than behind them.
  for (const std::size_t neighbour : open_doors(m_level))
  {
    const vec2 door = door_position(bounds, m_level.layout.rooms[neighbour].bounds) - origin;
    const room_role leads_to = m_level.layout.rooms[neighbour].role;

    // A door towards the boss is worth knowing about before walking through
    // it: the level ends there, and going back costs a room.
    const Color tint = (leads_to == room_role::boss)      ? Color{188, 84, 84, 255}
                       : (leads_to == room_role::station) ? Color{140, 200, 150, 255}
                                                          : Color{176, 128, 214, 255};

    // Drawn as a gap in the wall rather than as a dot: a doorway has to be
    // read from the far side of a room wider than the screen.
    const bool on_side = std::abs(door.x - (bounds.x - origin.x)) < 1.0f ||
                         std::abs(door.x - (bounds.x + bounds.width - origin.x)) < 1.0f;

    if (on_side)
    {
      DrawRectangle(static_cast<int>(door.x) - 3, static_cast<int>(door.y) - 14, 6, 28, tint);
    }
    else
    {
      DrawRectangle(static_cast<int>(door.x) - 14, static_cast<int>(door.y) - 3, 28, 6, tint);
    }
  }

  // Drawn before the entities so the player passes over it rather than
  // disappearing behind it.
  if (level_finished(m_level))
  {
    const Vector2 rift = to_raylib(m_exit.centre - origin);
    DrawCircleV(rift, m_exit.radius, Color{58, 40, 82, 255});
    DrawCircleV(rift, m_exit.radius * 0.45f, Color{176, 128, 214, 255});
  }

  for (auto [entity, place, shape, side] : m_world.view<const transform, const collider, const team>().each())
  {
    const bool is_shot = m_world.all_of<projectile>(entity);

    // An incoming shot has to be told apart from the enemy that fired it at a
    // glance, which is most of what makes a wall of bullets readable.
    const Color tint = (side.side == faction::player)
                           ? (is_shot ? Color{226, 205, 154, 255} : Color{198, 88, 78, 255})
                           : (is_shot ? Color{214, 118, 168, 255} : Color{92, 148, 138, 255});

    // Drawn larger than the collider for everything but the player, whose
    // sprite would otherwise lie about how easy they are to hit.
    const float drawn = m_world.all_of<player_controlled>(entity) ? 4.0f : shape.radius;

    // Blinking while invulnerable: a hit that takes a third of the life bar
    // must be visible, and the pause it grants has to read as one.
    const auto* shield = m_world.try_get<invulnerable>(entity);

    // Not while dashing, although that grants the same mercy: the blink says
    // "you were hit", and losing sight of the player during the one move that
    // crosses the screen would be the worst moment for it.
    const bool hurt = shield != nullptr && shield->remaining > 0.0f && !dashing(m_dash);
    const bool blinking = hurt && static_cast<int>(shield->remaining * 20.0f) % 2 == 0;

    if (!blinking)
    {
      DrawCircleV(to_raylib(interpolated(place, alpha) - origin), drawn, tint);
    }
  }

  if (player_alive())
  {
    const auto& player_health = m_world.get<health>(m_player);
    DrawText(TextFormat("HP %d", player_health.current), 4, 4, 10, Color{226, 205, 154, 255});

    // A dash that is not ready has to be visible without looking away from the
    // bullets, so it sits under the figure it belongs to rather than in a
    // corner of its own.
    const float ready =
        (m_profile.dash.cooldown <= 0.0f) ? 1.0f : 1.0f - std::min(1.0f, m_dash.cooldown / m_profile.dash.cooldown);

    DrawRectangle(4, 16, 24, 2, Color{48, 42, 58, 255});
    DrawRectangle(4, 16, static_cast<int>(24.0f * ready), 2,
                  (ready >= 1.0f) ? Color{176, 128, 214, 255} : Color{96, 74, 118, 255});
  }

  const char* tally = TextFormat("%zu / %zu", enemies_alive(m_world), m_fight.opened_with);
  const int tally_size = 10;

  // Right aligned, so the number moving does not shift the whole line.
  DrawText(tally, ctx().canvas->width() - MeasureText(tally, tally_size) - 4, 4, tally_size,
           (m_fight.state == encounter_state::cleared) ? Color{140, 200, 150, 255} : Color{160, 150, 170, 255});
  DrawText(m_biome.name.c_str(), 4, 154, 10, Color{120, 110, 130, 255});
  DrawText("ESC leave   -   F9 clear", 4, 166, 10, Color{120, 110, 130, 255});

  draw_minimap();

  if (!m_data_error.empty())
  {
    DrawText(m_data_error.c_str(), 4, 20, 10, Color{214, 118, 168, 255});
  }

  if (level_finished(m_level))
  {
    const char* way_out = "the rift is open";
    const int hint_size = 10;
    DrawText(way_out, (ctx().canvas->width() - MeasureText(way_out, hint_size)) / 2, 152, hint_size,
             Color{176, 128, 214, 255});
  }
}

} // namespace arpg
