// SPDX-License-Identifier: BSL-1.0

#include <screens/dungeon_screen.hpp>

#include <core/application.hpp>
#include <core/pixel_canvas.hpp>
#include <core/raylib_input.hpp>
#include <core/screen_manager.hpp>
#include <ecs/systems.hpp>

#include <raylib.h>

namespace arpg
{

namespace
{

constexpr float player_speed = 70.0f;
constexpr float focus_speed = 30.0f;
constexpr float bullet_speed = 220.0f;
constexpr float fire_interval = 0.12f;
constexpr float bullet_life = 3.0f;

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
  spawn_player();
  spawn_enemies();
}

void dungeon_screen::spawn_player()
{
  const pixel_canvas& canvas = *ctx().canvas;
  const vec2 middle{static_cast<float>(canvas.width()) * 0.5f, static_cast<float>(canvas.height()) * 0.75f};

  m_player = m_world.create();
  m_world.emplace<transform>(m_player, middle, middle);
  m_world.emplace<velocity>(m_player);
  m_world.emplace<collider>(m_player, player_hitbox);
  m_world.emplace<team>(m_player, faction::player);
  m_world.emplace<health>(m_player, 3, 3);
  m_world.emplace<player_controlled>(m_player);
}

void dungeon_screen::spawn_enemies()
{
  const pixel_canvas& canvas = *ctx().canvas;
  const float width = static_cast<float>(canvas.width());

  // Placeholders until the wave budget exists: enough to shoot at and to see
  // hits register.
  for (int i = 0; i < 4; ++i)
  {
    const vec2 spot{width * (0.2f + 0.2f * static_cast<float>(i)), 40.0f};

    const entt::entity foe = m_world.create();
    m_world.emplace<transform>(foe, spot, spot);
    m_world.emplace<collider>(foe, 6.0f);
    m_world.emplace<team>(foe, faction::enemy);
    m_world.emplace<health>(foe, 5, 5);
  }
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

  // The input layer does not know where the player stands, so an absolute aim
  // is turned into a heading here, where that is known.
  vec2 heading = aim.absolute ? normalized(aim.value - from) : aim.value;

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

  if (m_actions.consume(action::pause))
  {
    ctx().screens->pop();
    return;
  }

  steer_player();
  fire(dt);

  integrate_motion(m_world, dt);
  expire_lifetimes(m_world, dt);

  const pixel_canvas& canvas = *ctx().canvas;
  const viewport_rect room{0.0f, 0.0f, static_cast<float>(canvas.width()), static_cast<float>(canvas.height())};
  despawn_out_of_bounds(m_world, room, 16.0f);

  rebuild_spatial_hash(m_world, m_hash);
  resolve_projectile_hits(m_world, m_hash, m_scratch);
}

void dungeon_screen::render(float alpha)
{
  ClearBackground(Color{10, 8, 14, 255});

  for (auto [entity, place, shape, side] : m_world.view<const transform, const collider, const team>().each())
  {
    const bool is_shot = m_world.all_of<projectile>(entity);
    const Color tint = (side.side == faction::player) ? (is_shot ? Color{226, 205, 154, 255} : Color{198, 88, 78, 255})
                                                      : Color{92, 148, 138, 255};

    // Drawn larger than the collider for everything but the player, whose
    // sprite would otherwise lie about how easy they are to hit.
    const float drawn = m_world.all_of<player_controlled>(entity) ? 4.0f : shape.radius;
    DrawCircleV(to_raylib(interpolated(place, alpha)), drawn, tint);
  }

  const auto& player_health = m_world.get<health>(m_player);
  DrawText(TextFormat("HP %d", player_health.current), 4, 4, 10, Color{226, 205, 154, 255});
  DrawText("ESC leave", 4, 166, 10, Color{120, 110, 130, 255});
}

} // namespace arpg
