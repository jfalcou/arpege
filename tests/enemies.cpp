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
  const int corridor = arpg::combat_budget(60.0f * 30.0f, 1);
  const int arena = arpg::combat_budget(200.0f * 150.0f, 1);

  TTS_EXPECT(corridor > 0);
  TTS_EXPECT(arena > corridor);
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
