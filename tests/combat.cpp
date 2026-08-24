// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <ecs/systems.hpp>

namespace
{

/// Spawns a shot of @p side at @p position, heading nowhere by default.
entt::entity make_shot(entt::registry& world, arpg::faction side, arpg::vec2 position, int hurt = 1)
{
  const entt::entity shot = world.create();
  world.emplace<arpg::transform>(shot, position, position);
  world.emplace<arpg::collider>(shot, 2.0f);
  world.emplace<arpg::team>(shot, side);
  world.emplace<arpg::damage>(shot, hurt);
  world.emplace<arpg::projectile>(shot);
  return shot;
}

/// Spawns something that can be shot at.
entt::entity make_target(entt::registry& world, arpg::faction side, arpg::vec2 position, int hp = 10)
{
  const entt::entity target = world.create();
  world.emplace<arpg::transform>(target, position, position);
  world.emplace<arpg::collider>(target, 6.0f);
  world.emplace<arpg::team>(target, side);
  world.emplace<arpg::health>(target, hp, hp);
  return target;
}

int resolve(entt::registry& world, arpg::spatial_hash& hash)
{
  std::vector<entt::entity> scratch;
  arpg::rebuild_spatial_hash(world, hash);
  return arpg::resolve_projectile_hits(world, hash, scratch);
}

} // namespace

TTS_CASE("Motion advances the position and remembers the previous one")
{
  entt::registry world;
  const entt::entity mover = world.create();
  world.emplace<arpg::transform>(mover, arpg::vec2{10.0f, 10.0f}, arpg::vec2{10.0f, 10.0f});
  world.emplace<arpg::velocity>(mover, arpg::vec2{60.0f, 0.0f});

  arpg::integrate_motion(world, 1.0f / 60.0f);

  const auto& place = world.get<arpg::transform>(mover);
  TTS_EQUAL(place.previous.x, 10.0f);
  TTS_EQUAL(place.position.x, 11.0f);
};

TTS_CASE("Something without a velocity stays put")
{
  entt::registry world;
  const entt::entity still = world.create();
  world.emplace<arpg::transform>(still, arpg::vec2{5.0f, 5.0f}, arpg::vec2{5.0f, 5.0f});

  arpg::integrate_motion(world, 1.0f / 60.0f);

  TTS_EQUAL(world.get<arpg::transform>(still).position.x, 5.0f);
};

TTS_CASE("A lifetime runs out and takes the entity with it")
{
  entt::registry world;
  const entt::entity brief = world.create();
  world.emplace<arpg::lifetime>(brief, 0.1f);

  arpg::expire_lifetimes(world, 1.0f / 60.0f);
  TTS_EXPECT(world.valid(brief));

  arpg::expire_lifetimes(world, 1.0f);
  TTS_EXPECT_NOT(world.valid(brief));
};

TTS_CASE("A projectile leaving the play area is dropped")
{
  entt::registry world;
  const arpg::viewport_rect bounds{0.0f, 0.0f, 320.0f, 180.0f};

  const entt::entity inside = make_shot(world, arpg::faction::player, arpg::vec2{160.0f, 90.0f});
  const entt::entity outside = make_shot(world, arpg::faction::player, arpg::vec2{-100.0f, 90.0f});

  arpg::despawn_out_of_bounds(world, bounds, 16.0f);

  TTS_EXPECT(world.valid(inside));
  TTS_EXPECT_NOT(world.valid(outside));
};

TTS_CASE("The margin keeps a shot that is only just outside")
{
  entt::registry world;
  const arpg::viewport_rect bounds{0.0f, 0.0f, 320.0f, 180.0f};

  // Just past the edge, well within the margin: something spawned off screen
  // must be allowed to fly in.
  const entt::entity lingering = make_shot(world, arpg::faction::player, arpg::vec2{-8.0f, 90.0f});

  arpg::despawn_out_of_bounds(world, bounds, 16.0f);

  TTS_EXPECT(world.valid(lingering));
};

TTS_CASE("A non projectile stays even when far outside")
{
  entt::registry world;
  const arpg::viewport_rect bounds{0.0f, 0.0f, 320.0f, 180.0f};

  // An enemy walking in from off screen is not a stray shot.
  const entt::entity walker = make_target(world, arpg::faction::enemy, arpg::vec2{-500.0f, 90.0f});

  arpg::despawn_out_of_bounds(world, bounds, 16.0f);

  TTS_EXPECT(world.valid(walker));
};

namespace
{

/// A body that hurts on contact and survives doing so, unlike a projectile.
entt::entity make_walker(entt::registry& world, arpg::faction side, arpg::vec2 position, int hurt = 1)
{
  const entt::entity walker = world.create();
  world.emplace<arpg::transform>(walker, position, position);
  world.emplace<arpg::collider>(walker, 6.0f);
  world.emplace<arpg::team>(walker, side);
  world.emplace<arpg::damage>(walker, hurt);
  return walker;
}

int touch(entt::registry& world, arpg::spatial_hash& hash)
{
  std::vector<entt::entity> scratch;
  arpg::rebuild_spatial_hash(world, hash);
  return arpg::resolve_contact_damage(world, hash, scratch);
}

} // namespace

TTS_CASE("Walking into the opposing side hurts it")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  make_walker(world, arpg::faction::enemy, arpg::vec2{50.0f, 50.0f}, 1);
  const entt::entity victim = make_target(world, arpg::faction::player, arpg::vec2{52.0f, 50.0f}, 3);
  world.emplace<arpg::invulnerable>(victim, 0.0f, 0.8f);

  TTS_EQUAL(touch(world, hash), 1);
  TTS_EQUAL(world.get<arpg::health>(victim).current, 2);
};

TTS_CASE("What deals contact damage survives dealing it")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  // A projectile is spent on impact; a body walking into someone is not.
  const entt::entity walker = make_walker(world, arpg::faction::enemy, arpg::vec2{50.0f, 50.0f});
  const entt::entity victim = make_target(world, arpg::faction::player, arpg::vec2{52.0f, 50.0f}, 3);
  world.emplace<arpg::invulnerable>(victim);

  touch(world, hash);

  TTS_EXPECT(world.valid(walker));
  TTS_EXPECT(world.valid(victim));
};

TTS_CASE("A second touch inside the pause is ignored")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  make_walker(world, arpg::faction::enemy, arpg::vec2{50.0f, 50.0f}, 1);
  const entt::entity victim = make_target(world, arpg::faction::player, arpg::vec2{52.0f, 50.0f}, 3);
  world.emplace<arpg::invulnerable>(victim, 0.0f, 0.8f);

  TTS_EQUAL(touch(world, hash), 1);

  // Contact is tested every step: without the pause, standing against an enemy
  // would empty the bar in a fraction of a second.
  TTS_EQUAL(touch(world, hash), 0);
  TTS_EQUAL(touch(world, hash), 0);
  TTS_EQUAL(world.get<arpg::health>(victim).current, 2);
};

TTS_CASE("The pause runs out and the next touch lands")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  make_walker(world, arpg::faction::enemy, arpg::vec2{50.0f, 50.0f}, 1);
  const entt::entity victim = make_target(world, arpg::faction::player, arpg::vec2{52.0f, 50.0f}, 3);
  world.emplace<arpg::invulnerable>(victim, 0.0f, 0.8f);

  touch(world, hash);
  arpg::tick_invulnerability(world, 1.0f);

  TTS_EQUAL(touch(world, hash), 1);
  TTS_EQUAL(world.get<arpg::health>(victim).current, 1);
};

TTS_CASE("Invulnerability never counts below zero")
{
  entt::registry world;
  const entt::entity guarded = world.create();
  world.emplace<arpg::invulnerable>(guarded, 0.2f, 0.8f);

  arpg::tick_invulnerability(world, 5.0f);

  TTS_EQUAL(world.get<arpg::invulnerable>(guarded).remaining, 0.0f);
};

TTS_CASE("Contact spares its own side")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  make_walker(world, arpg::faction::enemy, arpg::vec2{50.0f, 50.0f});
  const entt::entity ally = make_target(world, arpg::faction::enemy, arpg::vec2{52.0f, 50.0f}, 3);
  world.emplace<arpg::invulnerable>(ally);

  TTS_EQUAL(touch(world, hash), 0);
  TTS_EQUAL(world.get<arpg::health>(ally).current, 3);
};

TTS_CASE("Contact kills what it empties")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  make_walker(world, arpg::faction::enemy, arpg::vec2{50.0f, 50.0f}, 5);
  const entt::entity victim = make_target(world, arpg::faction::player, arpg::vec2{52.0f, 50.0f}, 3);
  world.emplace<arpg::invulnerable>(victim);

  TTS_EQUAL(touch(world, hash), 1);
  TTS_EXPECT_NOT(world.valid(victim));
};

TTS_CASE("Something without invulnerability is still only hurt once per step")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  // No pause at all: it must still take one hit per step rather than one per
  // toucher standing on it.
  make_walker(world, arpg::faction::enemy, arpg::vec2{50.0f, 50.0f}, 1);
  const entt::entity victim = make_target(world, arpg::faction::player, arpg::vec2{52.0f, 50.0f}, 10);

  TTS_EQUAL(touch(world, hash), 1);
  TTS_EQUAL(world.get<arpg::health>(victim).current, 9);
};

TTS_CASE("A pass survives the grid still listing what an earlier one killed")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);
  std::vector<entt::entity> scratch;

  // An enemy about to die to a shot, standing on the player. The grid is built
  // once and both passes query it, which is what the game loop does: the
  // second one is handed an entity the first one destroyed.
  const entt::entity doomed = make_target(world, arpg::faction::enemy, arpg::vec2{50.0f, 50.0f}, 1);
  world.emplace<arpg::damage>(doomed, 1);

  make_shot(world, arpg::faction::player, arpg::vec2{50.0f, 50.0f}, 5);

  const entt::entity player = make_target(world, arpg::faction::player, arpg::vec2{52.0f, 50.0f}, 3);
  world.emplace<arpg::invulnerable>(player);

  arpg::rebuild_spatial_hash(world, hash);

  TTS_EQUAL(arpg::resolve_projectile_hits(world, hash, scratch), 1);
  TTS_EXPECT_NOT(world.valid(doomed));

  // Asking a destroyed entity for its components is not allowed, so this used
  // to abort rather than simply pass it over.
  TTS_EQUAL(arpg::resolve_contact_damage(world, hash, scratch), 0);
  TTS_EQUAL(world.get<arpg::health>(player).current, 3);
};

TTS_CASE("Contact can kill the player, whose entity then goes like any other")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  // Nothing marks the player as exempt: the entity is destroyed on death the
  // same way an enemy is, so every screen reading it has to ask first.
  make_walker(world, arpg::faction::enemy, arpg::vec2{50.0f, 50.0f}, 1);
  const entt::entity player = make_target(world, arpg::faction::player, arpg::vec2{52.0f, 50.0f}, 1);
  world.emplace<arpg::player_controlled>(player);
  world.emplace<arpg::invulnerable>(player);

  touch(world, hash);

  TTS_EXPECT_NOT(world.valid(player));
};

TTS_CASE("Whatever is confined is held inside the room")
{
  entt::registry world;
  const arpg::viewport_rect room{0.0f, 0.0f, 320.0f, 180.0f};

  const entt::entity strayed = world.create();
  world.emplace<arpg::transform>(strayed, arpg::vec2{-40.0f, 300.0f}, arpg::vec2{});
  world.emplace<arpg::collider>(strayed, 6.0f);
  world.emplace<arpg::confined>(strayed);

  arpg::confine_to_bounds(world, room);

  // The whole circle is held in, not its centre, so nothing sits half buried.
  const auto& place = world.get<arpg::transform>(strayed);
  TTS_EQUAL(place.position.x, 6.0f);
  TTS_EQUAL(place.position.y, 174.0f);
};

TTS_CASE("Something already inside is left where it is")
{
  entt::registry world;
  const arpg::viewport_rect room{0.0f, 0.0f, 320.0f, 180.0f};

  const entt::entity settled = world.create();
  world.emplace<arpg::transform>(settled, arpg::vec2{160.0f, 90.0f}, arpg::vec2{});
  world.emplace<arpg::collider>(settled, 6.0f);
  world.emplace<arpg::confined>(settled);

  arpg::confine_to_bounds(world, room);

  TTS_EQUAL(world.get<arpg::transform>(settled).position.x, 160.0f);
  TTS_EQUAL(world.get<arpg::transform>(settled).position.y, 90.0f);
};

TTS_CASE("A projectile is not held in, it is meant to leave")
{
  entt::registry world;
  const arpg::viewport_rect room{0.0f, 0.0f, 320.0f, 180.0f};

  const entt::entity shot = make_shot(world, arpg::faction::player, arpg::vec2{-40.0f, 90.0f});

  arpg::confine_to_bounds(world, room);

  TTS_EQUAL(world.get<arpg::transform>(shot).position.x, -40.0f);
};

TTS_CASE("A room narrower than its occupant centres it")
{
  entt::registry world;

  // The far edge would land before the near one, and clamping to a reversed
  // range is undefined.
  const arpg::viewport_rect cramped{0.0f, 0.0f, 4.0f, 4.0f};

  const entt::entity squeezed = world.create();
  world.emplace<arpg::transform>(squeezed, arpg::vec2{100.0f, 100.0f}, arpg::vec2{});
  world.emplace<arpg::collider>(squeezed, 10.0f);
  world.emplace<arpg::confined>(squeezed);

  arpg::confine_to_bounds(world, cramped);

  TTS_EQUAL(world.get<arpg::transform>(squeezed).position.x, 2.0f);
  TTS_EQUAL(world.get<arpg::transform>(squeezed).position.y, 2.0f);
};

TTS_CASE("A shot damages the opposing side and is spent")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  const entt::entity shot = make_shot(world, arpg::faction::player, arpg::vec2{50.0f, 50.0f}, 3);
  const entt::entity target = make_target(world, arpg::faction::enemy, arpg::vec2{51.0f, 50.0f}, 10);

  TTS_EQUAL(resolve(world, hash), 1);
  TTS_EXPECT_NOT(world.valid(shot));
  TTS_EQUAL(world.get<arpg::health>(target).current, 7);
};

TTS_CASE("A shot ignores its own side")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  const entt::entity shot = make_shot(world, arpg::faction::player, arpg::vec2{50.0f, 50.0f});
  const entt::entity friendly = make_target(world, arpg::faction::player, arpg::vec2{51.0f, 50.0f});

  TTS_EQUAL(resolve(world, hash), 0);
  TTS_EXPECT(world.valid(shot));
  TTS_EQUAL(world.get<arpg::health>(friendly).current, 10);
};

TTS_CASE("Two shots never test against each other")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  // Opposing sides, right on top of each other, and nothing must happen:
  // bullet against bullet is the pairing a bullet hell cannot afford.
  const entt::entity ours = make_shot(world, arpg::faction::player, arpg::vec2{50.0f, 50.0f});
  const entt::entity theirs = make_shot(world, arpg::faction::enemy, arpg::vec2{50.0f, 50.0f});

  TTS_EQUAL(resolve(world, hash), 0);
  TTS_EXPECT(world.valid(ours));
  TTS_EXPECT(world.valid(theirs));
};

TTS_CASE("A miss leaves everything alone")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  // Same neighbourhood, out of reach: the grid offers the candidate and the
  // distance test throws it out.
  const entt::entity shot = make_shot(world, arpg::faction::player, arpg::vec2{50.0f, 50.0f});
  const entt::entity target = make_target(world, arpg::faction::enemy, arpg::vec2{60.0f, 50.0f});

  TTS_EQUAL(resolve(world, hash), 0);
  TTS_EXPECT(world.valid(shot));
  TTS_EQUAL(world.get<arpg::health>(target).current, 10);
};

TTS_CASE("A target dies when its health runs out")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  const entt::entity shot = make_shot(world, arpg::faction::player, arpg::vec2{50.0f, 50.0f}, 10);
  const entt::entity target = make_target(world, arpg::faction::enemy, arpg::vec2{51.0f, 50.0f}, 10);

  TTS_EQUAL(resolve(world, hash), 1);
  TTS_EXPECT_NOT(world.valid(target));
  TTS_EXPECT_NOT(world.valid(shot));
};

TTS_CASE("Two shots finishing the same target do not destroy it twice")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  const entt::entity target = make_target(world, arpg::faction::enemy, arpg::vec2{50.0f, 50.0f}, 4);
  make_shot(world, arpg::faction::player, arpg::vec2{49.0f, 50.0f}, 4);
  make_shot(world, arpg::faction::player, arpg::vec2{51.0f, 50.0f}, 4);

  // The second shot must find a corpse rather than queue it for destruction a
  // second time, which would abort.
  TTS_EQUAL(resolve(world, hash), 1);
  TTS_EXPECT_NOT(world.valid(target));
};

TTS_CASE("A shot is spent on its first hit, not on a whole column")
{
  entt::registry world;
  arpg::spatial_hash hash(16.0f);

  make_shot(world, arpg::faction::player, arpg::vec2{50.0f, 50.0f}, 1);
  const entt::entity near_target = make_target(world, arpg::faction::enemy, arpg::vec2{51.0f, 50.0f}, 10);
  const entt::entity far_target = make_target(world, arpg::faction::enemy, arpg::vec2{53.0f, 50.0f}, 10);

  TTS_EQUAL(resolve(world, hash), 1);

  const int hurt = 20 - world.get<arpg::health>(near_target).current - world.get<arpg::health>(far_target).current;
  TTS_EQUAL(hurt, 1);
};
