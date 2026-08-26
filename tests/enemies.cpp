// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <ecs/systems.hpp>

#include <array>

namespace
{

// Named rather than positional: a field added to the archetype would otherwise
// slide every value one place along, silently.
constexpr arpg::enemy_archetype parasite{
    .cost = 5, .health = 2, .speed = 40.0f, .radius = 3.0f, .touch = 1, .sight = 90.0f, .reach = 20.0f};
constexpr arpg::enemy_archetype cultist{
    .cost = 10, .health = 5, .speed = 30.0f, .radius = 6.0f, .touch = 1, .sight = 90.0f, .reach = 24.0f};
constexpr arpg::enemy_archetype brute{
    .cost = 40, .health = 20, .speed = 18.0f, .radius = 10.0f, .touch = 2, .sight = 120.0f, .reach = 30.0f};

constexpr arpg::enemy_archetype shooter{.cost = 10,
                                        .health = 5,
                                        .speed = 20.0f,
                                        .radius = 6.0f,
                                        .touch = 1,
                                        .sight = 200.0f,
                                        .reach = 100.0f,
                                        .style = arpg::attack_style::ranged,
                                        .shots = {.interval = 1.0f, .speed = 60.0f, .radius = 2.0f, .damage = 1}};

constexpr std::array<arpg::enemy_archetype, 3> catalogue{parasite, cultist, brute};

/// Spawns an enemy that thinks on every step, so a test does not have to
/// wonder which slice it landed in.
entt::entity make_enemy(entt::registry& world, arpg::vec2 position, const arpg::enemy_archetype& kind = cultist)
{
  const entt::entity foe = world.create();
  world.emplace<arpg::transform>(foe, position, position);
  world.emplace<arpg::velocity>(foe);
  world.emplace<arpg::collider>(foe, kind.radius);
  world.emplace<arpg::team>(foe, arpg::faction::enemy);
  world.emplace<arpg::health>(foe, kind.health, kind.health);
  world.emplace<arpg::enemy_archetype>(foe, kind);
  world.emplace<arpg::enemy_brain>(foe);
  return foe;
}

/// Where a volley's headings land, reused rather than allocated per call.
std::vector<arpg::vec2> headings;

/// Spawns something that shoots and is already holding at range.
entt::entity make_shooter(entt::registry& world, arpg::vec2 position, float cooldown = 0.0f)
{
  const entt::entity foe = make_enemy(world, position, shooter);
  world.get<arpg::enemy_brain>(foe).state = arpg::enemy_state::attack;
  world.emplace<arpg::weapon>(foe, cooldown);
  return foe;
}

std::size_t count_shots(const entt::registry& world)
{
  return world.view<const arpg::projectile>().size();
}

int total_cost(const std::vector<std::size_t>& picked)
{
  int spent = 0;
  for (const std::size_t index : picked)
  {
    spent += catalogue[index].cost;
  }
  return spent;
}

} // namespace

TTS_CASE("A bigger room is worth more points")
{
  const int corridor = arpg::combat_budget(240.0f * 80.0f, 1);
  const int arena = arpg::combat_budget(640.0f * 360.0f, 1);

  TTS_EXPECT(corridor > 0);
  TTS_EXPECT(arena > corridor);
};

TTS_CASE("A cramped passage is not worth an ambush")
{
  // What matters is that nobody is fielded, not the exact figure: a short link
  // between two rooms is somewhere to breathe, and stuffing it would leave
  // nowhere to. Asserting the budget itself would break on every retuning.
  arpg::rng generator(11);
  const int budget = arpg::combat_budget(60.0f * 30.0f, 1);

  TTS_EQUAL(arpg::compose_wave(budget, catalogue, generator).size(), 0U);
};

TTS_CASE("The same room is harsher deeper down")
{
  const float area = 120.0f * 90.0f;

  TTS_EXPECT(arpg::combat_budget(area, 3) > arpg::combat_budget(area, 1));
};

TTS_CASE("A room with no floor is worth nothing")
{
  TTS_EQUAL(arpg::combat_budget(0.0f, 1), 0);
  TTS_EQUAL(arpg::combat_budget(-100.0f, 1), 0);
  TTS_EQUAL(arpg::combat_budget(1000.0f, 0), 0);
};

TTS_CASE("A wave never overspends its budget")
{
  arpg::rng generator(1);

  for (int budget = 1; budget < 200; budget += 7)
  {
    arpg::rng draw(static_cast<std::uint64_t>(budget));
    const auto picked = arpg::compose_wave(budget, catalogue, draw);
    TTS_EXPECT(total_cost(picked) <= budget);
  }
};

TTS_CASE("A wave spends what it can rather than stopping early")
{
  arpg::rng generator(3);
  const auto picked = arpg::compose_wave(100, catalogue, generator);

  // What is left over must be under the cheapest archetype, otherwise the
  // composition gave up with money still on the table.
  TTS_EXPECT(100 - total_cost(picked) < parasite.cost);
};

TTS_CASE("A budget under the cheapest enemy fields nobody")
{
  arpg::rng generator(3);
  TTS_EQUAL(arpg::compose_wave(4, catalogue, generator).size(), 0U);
  TTS_EQUAL(arpg::compose_wave(0, catalogue, generator).size(), 0U);
};

TTS_CASE("An empty catalogue fields nobody")
{
  arpg::rng generator(3);
  TTS_EQUAL(arpg::compose_wave(500, {}, generator).size(), 0U);
};

TTS_CASE("A free archetype does not compose an endless wave")
{
  // A cost of zero would be affordable forever and fill the room until memory
  // ran out, so the whole catalogue is refused instead.
  constexpr std::array<arpg::enemy_archetype, 1> broken{arpg::enemy_archetype{.cost = 0}};

  arpg::rng generator(3);
  TTS_EQUAL(arpg::compose_wave(500, broken, generator).size(), 0U);
};

TTS_CASE("The same seed composes the same wave")
{
  arpg::rng first(4242);
  arpg::rng second(4242);

  TTS_EQUAL(arpg::compose_wave(150, catalogue, first), arpg::compose_wave(150, catalogue, second));
};

TTS_CASE("Something holding at range fires")
{
  entt::registry world;
  const entt::entity foe = make_shooter(world, arpg::vec2{0.0f, 0.0f});

  TTS_EQUAL(arpg::fire_enemy_weapons(world, 1.0f / 60.0f, arpg::vec2{80.0f, 0.0f}, headings), 1);
  TTS_EQUAL(count_shots(world), 1U);

  // The shot belongs to whoever fired it, or it would hurt its own side.
  const auto shots = world.view<const arpg::projectile, const arpg::team>();
  for (const entt::entity shot : shots)
  {
    TTS_EQUAL(shots.get<const arpg::team>(shot).side, arpg::faction::enemy);
  }

  TTS_EXPECT(world.valid(foe));
};

TTS_CASE("A shot leaves along the line to its target")
{
  entt::registry world;
  make_shooter(world, arpg::vec2{0.0f, 0.0f});

  arpg::fire_enemy_weapons(world, 1.0f / 60.0f, arpg::vec2{100.0f, 0.0f}, headings);

  const auto shots = world.view<const arpg::velocity, const arpg::projectile>();
  for (const entt::entity shot : shots)
  {
    TTS_EXPECT(shots.get<const arpg::velocity>(shot).value.x > 0.0f);
  }
};

TTS_CASE("A shot starts clear of the body that fired it")
{
  entt::registry world;
  make_shooter(world, arpg::vec2{0.0f, 0.0f});

  arpg::fire_enemy_weapons(world, 1.0f / 60.0f, arpg::vec2{100.0f, 0.0f}, headings);

  // Spawned at the centre, a wide enemy would put the shot inside itself.
  const auto shots = world.view<const arpg::transform, const arpg::projectile>();
  for (const entt::entity shot : shots)
  {
    TTS_EXPECT(shots.get<const arpg::transform>(shot).position.x >= shooter.radius);
  }
};

TTS_CASE("A melee archetype never fires")
{
  entt::registry world;
  const entt::entity foe = make_enemy(world, arpg::vec2{0.0f, 0.0f}, brute);
  world.get<arpg::enemy_brain>(foe).state = arpg::enemy_state::attack;
  world.emplace<arpg::weapon>(foe);

  TTS_EQUAL(arpg::fire_enemy_weapons(world, 1.0f, arpg::vec2{20.0f, 0.0f}, headings), 0);
  TTS_EQUAL(count_shots(world), 0U);
};

TTS_CASE("Nothing fires while still closing in")
{
  entt::registry world;
  const entt::entity foe = make_shooter(world, arpg::vec2{0.0f, 0.0f});
  world.get<arpg::enemy_brain>(foe).state = arpg::enemy_state::chase;

  TTS_EQUAL(arpg::fire_enemy_weapons(world, 1.0f, arpg::vec2{80.0f, 0.0f}, headings), 0);
};

TTS_CASE("Nothing fires while it has not noticed anyone")
{
  entt::registry world;
  const entt::entity foe = make_shooter(world, arpg::vec2{0.0f, 0.0f});
  world.get<arpg::enemy_brain>(foe).state = arpg::enemy_state::idle;

  TTS_EQUAL(arpg::fire_enemy_weapons(world, 1.0f, arpg::vec2{80.0f, 0.0f}, headings), 0);
};

TTS_CASE("Firing waits out its interval")
{
  entt::registry world;
  make_shooter(world, arpg::vec2{0.0f, 0.0f});
  const arpg::vec2 target{80.0f, 0.0f};

  TTS_EQUAL(arpg::fire_enemy_weapons(world, 1.0f / 60.0f, target, headings), 1);

  // Called sixty times a second: without the interval a shooter would empty a
  // wall of bullets in one second.
  for (int i = 0; i < 30; ++i)
  {
    TTS_EQUAL(arpg::fire_enemy_weapons(world, 1.0f / 60.0f, target, headings), 0);
  }

  TTS_EQUAL(count_shots(world), 1U);
};

TTS_CASE("The interval runs out and it fires again")
{
  entt::registry world;
  make_shooter(world, arpg::vec2{0.0f, 0.0f});
  const arpg::vec2 target{80.0f, 0.0f};

  arpg::fire_enemy_weapons(world, 1.0f / 60.0f, target, headings);
  arpg::fire_enemy_weapons(world, 2.0f, target, headings);

  TTS_EQUAL(count_shots(world), 2U);
};

TTS_CASE("A staggered cooldown holds fire until it expires")
{
  entt::registry world;

  // Spawning a wave together would have it fire in one volley, so each one
  // starts somewhere inside its interval.
  make_shooter(world, arpg::vec2{0.0f, 0.0f}, 0.5f);

  TTS_EQUAL(arpg::fire_enemy_weapons(world, 1.0f / 60.0f, arpg::vec2{80.0f, 0.0f}, headings), 0);
  TTS_EQUAL(arpg::fire_enemy_weapons(world, 1.0f, arpg::vec2{80.0f, 0.0f}, headings), 1);
};

TTS_CASE("Standing exactly on the target does not fire a shot going nowhere")
{
  entt::registry world;
  make_shooter(world, arpg::vec2{40.0f, 40.0f});

  // No direction to fire along; a shot with no heading would sit on the muzzle
  // hurting whatever walked into it.
  TTS_EQUAL(arpg::fire_enemy_weapons(world, 1.0f, arpg::vec2{40.0f, 40.0f}, headings), 0);
};

TTS_CASE("An enemy ignores a player it cannot see")
{
  entt::registry world;
  const entt::entity foe = make_enemy(world, arpg::vec2{0.0f, 0.0f});

  arpg::advance_brains(world, 1.0f / 60.0f, 0, arpg::vec2{500.0f, 500.0f});

  TTS_EQUAL(world.get<arpg::enemy_brain>(foe).state, arpg::enemy_state::idle);
  TTS_EQUAL(arpg::length_squared(world.get<arpg::velocity>(foe).value), 0.0f);
};

TTS_CASE("An enemy that notices the player closes in")
{
  entt::registry world;
  const entt::entity foe = make_enemy(world, arpg::vec2{0.0f, 0.0f});

  arpg::advance_brains(world, 1.0f / 60.0f, 0, arpg::vec2{50.0f, 0.0f});

  TTS_EQUAL(world.get<arpg::enemy_brain>(foe).state, arpg::enemy_state::chase);
  TTS_EXPECT(world.get<arpg::velocity>(foe).value.x > 0.0f);
};

TTS_CASE("An enemy that has noticed the player never loses interest")
{
  entt::registry world;
  const entt::entity foe = make_enemy(world, arpg::vec2{0.0f, 0.0f});

  arpg::advance_brains(world, 1.0f / 60.0f, 0, arpg::vec2{50.0f, 0.0f});

  // Far beyond sight: retreating out of view must not put the room back to
  // sleep, or the player could pick the crowd apart from the doorway.
  arpg::advance_brains(world, 1.0f / 60.0f, 4, arpg::vec2{500.0f, 0.0f});

  TTS_EQUAL(world.get<arpg::enemy_brain>(foe).state, arpg::enemy_state::chase);
};

TTS_CASE("An enemy holds position once within reach")
{
  entt::registry world;
  const entt::entity foe = make_enemy(world, arpg::vec2{0.0f, 0.0f});

  arpg::advance_brains(world, 1.0f / 60.0f, 0, arpg::vec2{10.0f, 0.0f});
  arpg::advance_brains(world, 1.0f / 60.0f, 4, arpg::vec2{10.0f, 0.0f});

  TTS_EQUAL(world.get<arpg::enemy_brain>(foe).state, arpg::enemy_state::attack);
  TTS_EQUAL(arpg::length_squared(world.get<arpg::velocity>(foe).value), 0.0f);
};

TTS_CASE("Leaving reach does not flip the state every other step")
{
  entt::registry world;
  const entt::entity foe = make_enemy(world, arpg::vec2{0.0f, 0.0f});

  arpg::advance_brains(world, 1.0f / 60.0f, 0, arpg::vec2{10.0f, 0.0f});
  arpg::advance_brains(world, 1.0f / 60.0f, 4, arpg::vec2{10.0f, 0.0f});

  // Just past reach: without hysteresis this would swing back to chasing, and
  // the enemy would stutter between the two forever.
  arpg::advance_brains(world, 1.0f / 60.0f, 8, arpg::vec2{26.0f, 0.0f});

  TTS_EQUAL(world.get<arpg::enemy_brain>(foe).state, arpg::enemy_state::attack);
};

TTS_CASE("Only its own slice makes an enemy reconsider")
{
  entt::registry world;
  const entt::entity foe = make_enemy(world, arpg::vec2{0.0f, 0.0f});
  world.get<arpg::enemy_brain>(foe).slice = 1;

  // Its slice is 1, so a step of 0 is somebody else's turn to think.
  arpg::advance_brains(world, 1.0f / 60.0f, 0, arpg::vec2{50.0f, 0.0f});
  TTS_EQUAL(world.get<arpg::enemy_brain>(foe).state, arpg::enemy_state::idle);

  arpg::advance_brains(world, 1.0f / 60.0f, 1, arpg::vec2{50.0f, 0.0f});
  TTS_EQUAL(world.get<arpg::enemy_brain>(foe).state, arpg::enemy_state::chase);
};

TTS_CASE("A skipped step still ages the state")
{
  entt::registry world;
  const entt::entity foe = make_enemy(world, arpg::vec2{0.0f, 0.0f});
  world.get<arpg::enemy_brain>(foe).slice = 1;

  arpg::advance_brains(world, 0.5f, 0, arpg::vec2{500.0f, 0.0f});

  // Time passes for everyone, whether or not it was their turn to think.
  TTS_EQUAL(world.get<arpg::enemy_brain>(foe).state_timer, 0.5f);
};

TTS_CASE("A body is placed inside the room it belongs to")
{
  arpg::rng generator(5);
  const arpg::viewport_rect room{100.0f, 50.0f, 400.0f, 300.0f};

  for (int i = 0; i < 200; ++i)
  {
    const arpg::vec2 spot = arpg::pick_spawn(generator, room, 8.0f, {}, 60.0f);

    TTS_EXPECT(spot.x >= room.x + 8.0f);
    TTS_EXPECT(spot.x <= room.x + room.width - 8.0f);
    TTS_EXPECT(spot.y >= room.y + 8.0f);
    TTS_EXPECT(spot.y <= room.y + room.height - 8.0f);
  }
};

TTS_CASE("Nothing is placed on a doorway or on the player")
{
  arpg::rng generator(11);
  const arpg::viewport_rect room{0.0f, 0.0f, 400.0f, 300.0f};

  // A doorway and where the player sets foot: arriving inside an enemy is not
  // difficulty, it is a hit nobody could have avoided.
  const std::array<arpg::vec2, 2> keep_clear{arpg::vec2{0.0f, 150.0f}, arpg::vec2{200.0f, 150.0f}};

  for (int i = 0; i < 200; ++i)
  {
    const arpg::vec2 spot = arpg::pick_spawn(generator, room, 6.0f, keep_clear, 60.0f);

    for (const arpg::vec2 avoid : keep_clear)
    {
      TTS_EXPECT(arpg::length_squared(spot - avoid) >= 60.0f * 60.0f);
    }
  }
};

TTS_CASE("A room with nowhere clear still yields a spot rather than none")
{
  arpg::rng generator(3);
  const arpg::viewport_rect cramped{0.0f, 0.0f, 60.0f, 60.0f};

  // The clearance covers the whole room. A wave quietly missing its enemies
  // would be a worse bug than one standing a little too close, so it gives up
  // and places anyway.
  const std::array<arpg::vec2, 1> keep_clear{arpg::vec2{30.0f, 30.0f}};
  const arpg::vec2 spot = arpg::pick_spawn(generator, cramped, 4.0f, keep_clear, 500.0f);

  TTS_EXPECT(spot.x >= cramped.x);
  TTS_EXPECT(spot.x <= cramped.x + cramped.width);
};

TTS_CASE("A room narrower than the body it receives puts it against the wall")
{
  arpg::rng generator(2);
  const arpg::viewport_rect sliver{0.0f, 0.0f, 4.0f, 4.0f};

  const arpg::vec2 spot = arpg::pick_spawn(generator, sliver, 20.0f, {}, 10.0f);

  // Nothing sensible fits, and an empty span must not read as a negative one.
  TTS_EQUAL(spot.x, 20.0f);
  TTS_EQUAL(spot.y, 20.0f);
};

TTS_CASE("The same seed places the same body")
{
  arpg::rng once(77);
  arpg::rng twice(77);
  const arpg::viewport_rect room{0.0f, 0.0f, 400.0f, 300.0f};

  const arpg::vec2 a = arpg::pick_spawn(once, room, 6.0f, {}, 40.0f);
  const arpg::vec2 b = arpg::pick_spawn(twice, room, 6.0f, {}, 40.0f);

  TTS_EQUAL(a.x, b.x);
  TTS_EQUAL(a.y, b.y);
};

TTS_CASE("A weight decides how often a kind is offered")
{
  arpg::rng generator(4);

  // Two archetypes of the same price, so nothing but the weight can tell them
  // apart in the draw.
  constexpr arpg::enemy_archetype common{
      .cost = 5, .health = 2, .speed = 40.0f, .radius = 3.0f, .touch = 1, .sight = 90.0f, .reach = 20.0f};
  constexpr arpg::enemy_archetype rare{
      .cost = 5, .health = 2, .speed = 40.0f, .radius = 3.0f, .touch = 1, .sight = 90.0f, .reach = 20.0f};

  const std::array<arpg::enemy_archetype, 2> pair{common, rare};
  const std::array<int, 2> weights{9, 1};

  int firsts = 0;
  int total = 0;

  for (int round = 0; round < 200; ++round)
  {
    for (const std::size_t index : arpg::compose_wave(100, pair, weights, generator))
    {
      firsts += (index == 0) ? 1 : 0;
      ++total;
    }
  }

  // Nine to one, so anywhere near half would mean the weights are ignored.
  TTS_EXPECT(firsts > total * 3 / 4);
  TTS_EXPECT(firsts < total);
};

TTS_CASE("A catalogue handed no weights is offered evenly")
{
  arpg::rng generator(6);
  const std::array<int, 0> none{};

  int firsts = 0;
  int total = 0;

  for (int round = 0; round < 200; ++round)
  {
    for (const std::size_t index : arpg::compose_wave(100, catalogue, none, generator))
    {
      firsts += (index == 0) ? 1 : 0;
      ++total;
    }
  }

  TTS_EXPECT(firsts > 0);
  TTS_EXPECT(firsts < total);
};

TTS_CASE("A weighted wave still spends what it was given")
{
  arpg::rng generator(8);
  const std::array<int, 3> weights{1, 1, 1};

  int spent = 0;

  for (const std::size_t index : arpg::compose_wave(120, catalogue, weights, generator))
  {
    spent += catalogue[index].cost;
  }

  // The weights change who is drawn, never how much a room can afford.
  TTS_EXPECT(spent <= 120);
  TTS_EXPECT(spent > 120 - catalogue[0].cost);
};

TTS_CASE("The state a creature is in decides the animation it plays")
{
  entt::registry world;

  arpg::enemy_archetype dressed = cultist;
  dressed.drawn = true;
  dressed.clips[static_cast<std::size_t>(arpg::enemy_state::idle)] = 7;
  dressed.clips[static_cast<std::size_t>(arpg::enemy_state::chase)] = 8;
  dressed.clips[static_cast<std::size_t>(arpg::enemy_state::attack)] = 9;

  const entt::entity foe = make_enemy(world, arpg::vec2{}, dressed);
  world.emplace<arpg::appearance>(foe);

  arpg::dress_enemies(world);
  TTS_EQUAL(world.get<arpg::appearance>(foe).clip, 7);

  world.get<arpg::enemy_brain>(foe).state = arpg::enemy_state::chase;
  world.get<arpg::appearance>(foe).elapsed = 3.0f;
  arpg::dress_enemies(world);

  TTS_EQUAL(world.get<arpg::appearance>(foe).clip, 8);

  // Started over rather than carried across: keeping the elapsed time would
  // drop into the middle of a lunge.
  TTS_EQUAL(world.get<arpg::appearance>(foe).elapsed, 0.0f);
};

TTS_CASE("Staying in a state does not restart what is playing")
{
  entt::registry world;

  arpg::enemy_archetype dressed = cultist;
  dressed.drawn = true;

  const entt::entity foe = make_enemy(world, arpg::vec2{}, dressed);
  world.emplace<arpg::appearance>(foe);

  arpg::dress_enemies(world);
  world.get<arpg::appearance>(foe).elapsed = 0.5f;
  arpg::dress_enemies(world);

  TTS_EQUAL(world.get<arpg::appearance>(foe).elapsed, 0.5f);
};

TTS_CASE("A creature with no sheet is left alone rather than drawn as nothing")
{
  entt::registry world;

  // A roster loaded without its pictures still fields enemies; they fall back
  // on the shapes that stood in for them.
  const entt::entity foe = make_enemy(world, arpg::vec2{}, cultist);
  world.emplace<arpg::appearance>(foe, std::uint16_t{3}, std::uint16_t{4}, 1.5f);

  arpg::dress_enemies(world);

  TTS_EQUAL(world.get<arpg::appearance>(foe).clip, 4);
  TTS_EQUAL(world.get<arpg::appearance>(foe).elapsed, 1.5f);
};

TTS_CASE("Time passes for a picture the same way it does for the world")
{
  entt::registry world;
  const entt::entity thing = world.create();
  world.emplace<arpg::appearance>(thing);

  arpg::advance_appearances(world, 0.25f);
  arpg::advance_appearances(world, 0.25f);

  TTS_EQUAL(world.get<arpg::appearance>(thing).elapsed, 0.5f);
};
