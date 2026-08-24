// SPDX-License-Identifier: BSL-1.0

#include <screens/dungeon_screen.hpp>

#include <core/application.hpp>
#include <core/events.hpp>
#include <core/pixel_canvas.hpp>
#include <core/raylib_input.hpp>
#include <core/screen_manager.hpp>
#include <ecs/systems.hpp>

#include <raylib.h>

#include <array>

namespace arpg
{

namespace
{

constexpr float player_speed = 70.0f;
constexpr float focus_speed = 30.0f;
constexpr float bullet_speed = 220.0f;
constexpr float fire_interval = 0.12f;
constexpr float bullet_life = 1.2f;

/// How far a player shot actually carries. The room is wider than the screen,
/// so a shot must die around the edge of the view rather than cross the whole
/// room and kill what the player cannot even see.
constexpr float player_range = bullet_speed * bullet_life;

/// Every archetype wakes at the same distance, and that distance must stay
/// above player_range: an enemy the player can already hit but that has not
/// noticed anything is a free kill, not an encounter.
constexpr float awareness = player_range + 24.0f;

/// Where the roster is read from, relative to the executable.
constexpr const char* roster_path = "assets/data/enemies.lua";

/// How much larger than the screen a room is, for now. Rooms come from the
/// generator later and carry their own size.
constexpr float room_scale = 2.0f;

/// How eagerly the view catches up, in units per second. Tight enough to feel
/// attached, loose enough not to judder on a fixed step.
constexpr float camera_stiffness = 8.0f;

/// A few pixels, far smaller than the sprite: the player must be able to thread
/// a wall of bullets that visually looks impassable.
constexpr float player_hitbox = 2.0f;

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
  m_roster = load_enemies_from(m_scripts, roster_path, player_range);

  spawn_player();
  spawn_wave();
}

viewport_rect dungeon_screen::room() const
{
  const vec2 screen = view();
  return viewport_rect{0.0f, 0.0f, screen.x * room_scale, screen.y * room_scale};
}

vec2 dungeon_screen::view() const
{
  const pixel_canvas& canvas = *ctx().canvas;
  return vec2{static_cast<float>(canvas.width()), static_cast<float>(canvas.height())};
}

void dungeon_screen::spawn_player()
{
  const viewport_rect bounds = room();
  const vec2 middle{bounds.width * 0.5f, bounds.height * 0.75f};

  m_player = m_world.create();
  m_world.emplace<transform>(m_player, middle, middle);
  m_world.emplace<velocity>(m_player);
  m_world.emplace<collider>(m_player, player_hitbox);
  m_world.emplace<team>(m_player, faction::player);
  m_world.emplace<health>(m_player, 3, 3);
  m_world.emplace<invulnerable>(m_player);
  m_world.emplace<confined>(m_player);
  m_world.emplace<player_controlled>(m_player);

  // Snapped rather than eased on the first step, or the room would slide into
  // place from a corner every time it opens.
  m_camera.centre = follow_camera(middle, middle, room(), view(), 0.0f, 0.0f);
}

void dungeon_screen::spawn_wave()
{
  if (!m_roster.valid())
  {
    return;
  }

  const viewport_rect bounds = room();
  const float width = bounds.width;
  const float height = bounds.height;

  // The room pays for its own enemies: what fits in the budget is what shows
  // up, so a bigger room is dangerous in proportion rather than by luck.
  const int budget = combat_budget(width * height, 1);
  const auto composition = compose_wave(budget, m_roster.kinds, m_generator);

  std::uint8_t slice = 0;

  for (const std::size_t index : composition)
  {
    const enemy_archetype& kind = m_roster.kinds[index];

    // Spawned across the upper half, away from where the player starts.
    const vec2 spot{m_generator.unit() * (width - 2.0f * kind.radius) + kind.radius,
                    m_generator.unit() * height * 0.45f + kind.radius};

    const entt::entity foe = m_world.create();
    m_world.emplace<transform>(foe, spot, spot);
    m_world.emplace<velocity>(foe);
    m_world.emplace<collider>(foe, kind.radius);
    m_world.emplace<team>(foe, faction::enemy);
    m_world.emplace<health>(foe, kind.health, kind.health);
    m_world.emplace<enemy_archetype>(foe, kind);
    m_world.emplace<damage>(foe, kind.touch);

    // Staggered, so a wave that spawns together does not fire in one volley.
    m_world.emplace<weapon>(foe, m_generator.unit() * kind.fire_interval);
    m_world.emplace<confined>(foe);

    // Dealt round-robin so the crowd is spread evenly over the thinking
    // rounds instead of everyone landing in the same one.
    m_world.emplace<enemy_brain>(foe, enemy_state::idle, 0.0f, slice);
    slice = static_cast<std::uint8_t>((slice + 1) % 4);
  }

  m_fight.opened_with = composition.size();
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

void dungeon_screen::steer_player()
{
  const action_set held = m_bindings.resolve(*ctx().input);
  const vec2 heading = movement_direction(held, ctx().input->left_stick);

  // Focus trades speed for precision, and is where the tiny hitbox earns its
  // keep.
  const float speed = m_actions.held(action::focus) ? focus_speed : player_speed;

  m_world.get<velocity>(m_player).value = heading * speed;
}

void dungeon_screen::fire(float dt)
{
  m_fire_cooldown -= dt;

  if (!m_actions.held(action::shoot) || m_fire_cooldown > 0.0f)
  {
    return;
  }

  m_fire_cooldown = fire_interval;

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
  m_world.emplace<velocity>(shot, heading * bullet_speed);
  m_world.emplace<collider>(shot, 1.5f);
  m_world.emplace<team>(shot, faction::player);
  m_world.emplace<damage>(shot, 1);
  m_world.emplace<lifetime>(shot, bullet_life);
  m_world.emplace<projectile>(shot);
}

void dungeon_screen::update(float dt)
{
  m_actions.advance(m_bindings.resolve(*ctx().input));

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

  steer_player();
  fire(dt);

  const vec2 player_at = m_world.get<transform>(m_player).position;

  advance_brains(m_world, dt, m_step, player_at);
  fire_enemy_weapons(m_world, dt, player_at);
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

  if (m_fight.state == encounter_state::cleared && enter_portal(m_exit, m_world.get<transform>(m_player).position))
  {
    ctx().screens->pop();
  }
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

  // Drawn before the entities so the player passes over it rather than
  // disappearing behind it.
  if (m_fight.state == encounter_state::cleared)
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
    const bool blinking =
        shield != nullptr && shield->remaining > 0.0f && static_cast<int>(shield->remaining * 20.0f) % 2 == 0;

    if (!blinking)
    {
      DrawCircleV(to_raylib(interpolated(place, alpha) - origin), drawn, tint);
    }
  }

  if (player_alive())
  {
    const auto& player_health = m_world.get<health>(m_player);
    DrawText(TextFormat("HP %d", player_health.current), 4, 4, 10, Color{226, 205, 154, 255});
  }

  const char* tally = TextFormat("%zu / %zu", enemies_alive(m_world), m_fight.opened_with);
  const int tally_size = 10;

  // Right aligned, so the number moving does not shift the whole line.
  DrawText(tally, ctx().canvas->width() - MeasureText(tally, tally_size) - 4, 4, tally_size,
           (m_fight.state == encounter_state::cleared) ? Color{140, 200, 150, 255} : Color{160, 150, 170, 255});
  DrawText("ESC leave   -   F9 clear", 4, 166, 10, Color{120, 110, 130, 255});

  if (!m_roster.valid())
  {
    DrawText(m_roster.error.c_str(), 4, 20, 10, Color{214, 118, 168, 255});
  }

  if (m_fight.state == encounter_state::cleared)
  {
    const char* way_out = "the rift is open";
    const int hint_size = 10;
    DrawText(way_out, (ctx().canvas->width() - MeasureText(way_out, hint_size)) / 2, 152, hint_size,
             Color{176, 128, 214, 255});
  }
}

} // namespace arpg
