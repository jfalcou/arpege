// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <ecs/components.hpp>
#include <ecs/encounter.hpp>
#include <ecs/enemy.hpp>

namespace
{

constexpr arpg::enemy_archetype parasite{
    .cost = 5, .health = 2, .speed = 40.0f, .radius = 3.0f, .touch = 1, .sight = 90.0f, .reach = 20.0f};

entt::entity make_enemy(entt::registry& world)
{
  const entt::entity foe = world.create();
  world.emplace<arpg::enemy_archetype>(foe, parasite);
  world.emplace<arpg::health>(foe, parasite.health, parasite.health);
  return foe;
}

} // namespace

TTS_CASE("A room with something left in it is not cleared")
{
  entt::registry world;
  make_enemy(world);

  arpg::encounter fight;

  TTS_EXPECT_NOT(arpg::advance_encounter(fight, world));
  TTS_EQUAL(fight.state, arpg::encounter_state::fighting);
  TTS_EQUAL(arpg::enemies_alive(world), 1U);
};

TTS_CASE("Clearing a room is reported once and only once")
{
  entt::registry world;
  const entt::entity foe = make_enemy(world);

  arpg::encounter fight;
  arpg::advance_encounter(fight, world);

  world.destroy(foe);

  // The step the last one falls on is the step that carries the news, and the
  // caller must be able to raise its event without tracking whether it already
  // did so.
  TTS_EXPECT(arpg::advance_encounter(fight, world));
  TTS_EQUAL(fight.state, arpg::encounter_state::cleared);

  TTS_EXPECT_NOT(arpg::advance_encounter(fight, world));
  TTS_EXPECT_NOT(arpg::advance_encounter(fight, world));
};

TTS_CASE("An empty room is cleared straight away")
{
  entt::registry world;
  arpg::encounter fight;

  TTS_EXPECT(arpg::advance_encounter(fight, world));
};

TTS_CASE("Reinforcements do not reopen a cleared room")
{
  entt::registry world;
  arpg::encounter fight;

  arpg::advance_encounter(fight, world);
  make_enemy(world);

  TTS_EXPECT_NOT(arpg::advance_encounter(fight, world));
  TTS_EQUAL(fight.state, arpg::encounter_state::cleared);
};

TTS_CASE("Standing where the rift opens is not walking into it")
{
  arpg::exit_portal way{.centre = arpg::vec2{0.0f, 0.0f}, .radius = 10.0f};

  // The portal appears under the player, who has not moved: leaving must take
  // a deliberate step out and back in.
  TTS_EXPECT_NOT(arpg::enter_portal(way, arpg::vec2{0.0f, 0.0f}));
  TTS_EXPECT_NOT(arpg::enter_portal(way, arpg::vec2{0.0f, 0.0f}));

  arpg::enter_portal(way, arpg::vec2{40.0f, 0.0f});

  TTS_EXPECT(arpg::enter_portal(way, arpg::vec2{0.0f, 0.0f}));
};

TTS_CASE("The rift takes the player on the step they enter it")
{
  arpg::exit_portal way{.centre = arpg::vec2{100.0f, 0.0f}, .radius = 10.0f};

  TTS_EXPECT_NOT(arpg::enter_portal(way, arpg::vec2{0.0f, 0.0f}));
  TTS_EXPECT(arpg::enter_portal(way, arpg::vec2{96.0f, 0.0f}));

  // Still inside on the next step, which must not read as a second entry.
  TTS_EXPECT_NOT(arpg::enter_portal(way, arpg::vec2{96.0f, 0.0f}));
};

TTS_CASE("The edge of the rift is outside it")
{
  arpg::exit_portal way{.centre = arpg::vec2{0.0f, 0.0f}, .radius = 10.0f, .armed = true};

  TTS_EXPECT_NOT(arpg::enter_portal(way, arpg::vec2{10.0f, 0.0f}));
  TTS_EXPECT(arpg::enter_portal(way, arpg::vec2{9.9f, 0.0f}));
};
