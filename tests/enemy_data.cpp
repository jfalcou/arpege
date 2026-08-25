// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <data/enemy_data.hpp>

namespace
{

constexpr float minimum_sight = 100.0f;

/// A roster that passes every check, for tests that alter one thing in it.
constexpr const char* sound_roster = R"(
  return {
    { name = "parasite", cost = 5, health = 2, speed = 58, radius = 3,
      touch = 1, sight = 288, reach = 10, style = "melee" },
    { name = "cultist", cost = 10, health = 5, speed = 40, radius = 6,
      touch = 1, sight = 288, reach = 120, style = "ranged",
      shots = { bullets = 3, arc = 24, interval = 1.4, speed = 78 } },
  }
)";

} // namespace

TTS_CASE("A roster is read with its names and its figures")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, sound_roster, minimum_sight);

  TTS_EXPECT(roster.valid()) << roster.error;
  TTS_EQUAL(roster.kinds.size(), 2U);
  TTS_EQUAL(roster.names.size(), 2U);

  TTS_EQUAL(roster.names[0], std::string{"parasite"});
  TTS_EQUAL(roster.kinds[0].cost, 5);
  TTS_EQUAL(roster.kinds[0].health, 2);
  TTS_EQUAL(roster.kinds[0].speed, 58.0f);
  TTS_EQUAL(roster.kinds[0].style, arpg::attack_style::melee);

  TTS_EQUAL(roster.names[1], std::string{"cultist"});
  TTS_EQUAL(roster.kinds[1].style, arpg::attack_style::ranged);
  TTS_EQUAL(roster.kinds[1].reach, 120.0f);
  TTS_EQUAL(roster.kinds[1].shots.speed, 78.0f);
  TTS_EQUAL(roster.kinds[1].shots.bullets, 3);
  TTS_EQUAL(roster.kinds[1].shots.arc, 24.0f);
}; 

TTS_CASE("A file that is not Lua at all is reported, not guessed at")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, "return {{{", minimum_sight);

  TTS_EXPECT_NOT(roster.valid());
  TTS_EXPECT(roster.kinds.empty());
};

TTS_CASE("A script that returns something else is refused")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, "return 42", minimum_sight);

  TTS_EXPECT_NOT(roster.valid());
};

TTS_CASE("An archetype that costs nothing is refused")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, R"(
    return {
      { name = "ghost", cost = 0, health = 1, speed = 10, radius = 2,
        touch = 1, sight = 288, reach = 10, style = "melee" },
    }
  )", minimum_sight);

  // Composing a wave buys archetypes until the budget runs out, so one that
  // costs nothing is bought forever.
  TTS_EXPECT_NOT(roster.valid());
};

TTS_CASE("An archetype that wakes closer than the player can strike is refused")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, R"(
    return {
      { name = "dozy", cost = 5, health = 2, speed = 10, radius = 2,
        touch = 1, sight = 40, reach = 10, style = "melee" },
    }
  )", minimum_sight);

  TTS_EXPECT_NOT(roster.valid());
};

TTS_CASE("A shooter with no way to shoot is refused")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, R"(
    return {
      { name = "mute", cost = 5, health = 2, speed = 10, radius = 2,
        touch = 1, sight = 288, reach = 90, style = "ranged" },
    }
  )", minimum_sight);

  TTS_EXPECT_NOT(roster.valid());
};

TTS_CASE("A volley is described by a count, an arc and a rotation")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, R"(
    return {
      { name = "turret", cost = 20, health = 6, speed = 10, radius = 5,
        touch = 1, sight = 288, reach = 150, style = "ranged",
        shots = { aim = "fixed", bullets = 8, arc = 360, spin = 11,
                  interval = 0.8, speed = 50, radius = 2, damage = 1, life = 6 } },
    }
  )", minimum_sight);

  TTS_EXPECT(roster.valid()) << roster.error;
  TTS_EQUAL(roster.kinds[0].shots.aim, arpg::aim_mode::fixed);
  TTS_EQUAL(roster.kinds[0].shots.bullets, 8);
  TTS_EQUAL(roster.kinds[0].shots.arc, 360.0f);
  TTS_EQUAL(roster.kinds[0].shots.spin, 11.0f);
  TTS_EQUAL(roster.kinds[0].shots.life, 6.0f);
};

TTS_CASE("An unknown way of aiming is refused")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, R"(
    return {
      { name = "seer", cost = 20, health = 6, speed = 10, radius = 5,
        touch = 1, sight = 288, reach = 150, style = "ranged",
        shots = { aim = "predictive", bullets = 1, interval = 1, speed = 50 } },
    }
  )", minimum_sight);

  TTS_EXPECT_NOT(roster.valid());
};

TTS_CASE("A volley of ten thousand bullets is taken for a typo")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, R"(
    return {
      { name = "storm", cost = 20, health = 6, speed = 10, radius = 5,
        touch = 1, sight = 288, reach = 150, style = "ranged",
        shots = { bullets = 10000, arc = 360, interval = 1, speed = 50 } },
    }
  )", minimum_sight);

  // A file that reloads while it is being edited must not put ten thousand
  // entities in the room: the result would be a freeze rather than a
  // readable mistake.
  TTS_EXPECT_NOT(roster.valid());
};

TTS_CASE("A name used twice is refused")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, R"(
    return {
      { name = "twin", cost = 5, health = 2, speed = 10, radius = 2,
        touch = 1, sight = 288, reach = 10, style = "melee" },
      { name = "twin", cost = 7, health = 3, speed = 10, radius = 2,
        touch = 1, sight = 288, reach = 10, style = "melee" },
    }
  )", minimum_sight);

  // Rooms will field enemies by name, and a shadowed one would be silently
  // unreachable.
  TTS_EXPECT_NOT(roster.valid());
};

TTS_CASE("An unknown style is refused rather than taken for melee")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, R"(
    return {
      { name = "odd", cost = 5, health = 2, speed = 10, radius = 2,
        touch = 1, sight = 288, reach = 10, style = "psychic" },
    }
  )", minimum_sight);

  TTS_EXPECT_NOT(roster.valid());
};

TTS_CASE("One bad entry throws the whole roster away")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, R"(
    return {
      { name = "sound", cost = 5, health = 2, speed = 10, radius = 2,
        touch = 1, sight = 288, reach = 10, style = "melee" },
      { name = "broken", cost = -1, health = 2, speed = 10, radius = 2,
        touch = 1, sight = 288, reach = 10, style = "melee" },
    }
  )", minimum_sight);

  // Half a roster would field a wave nobody designed, and the mistake would
  // read as a strange fight rather than as a message.
  TTS_EXPECT_NOT(roster.valid());
  TTS_EXPECT(roster.kinds.empty());
};

TTS_CASE("An empty roster is an error, not an empty room")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, "return {}", minimum_sight);

  TTS_EXPECT_NOT(roster.valid());
};

TTS_CASE("A file that is not there is an error like any other")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies_from(host, "no/such/file.lua", minimum_sight);

  TTS_EXPECT_NOT(roster.valid());
};

TTS_CASE("A data file may compute rather than only state")
{
  arpg::script_host host;
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, R"(
    local swarm = { cost = 5, health = 2, speed = 58, radius = 3,
                    touch = 1, sight = 288, reach = 10, style = "melee" }

    local roster = {}
    for i = 1, 3 do
      local kind = {}
      for key, value in pairs(swarm) do kind[key] = value end
      kind.name = "parasite_" .. i
      kind.health = i
      roster[i] = kind
    end
    return roster
  )", minimum_sight);

  // What Lua buys over a table of literals: a family of related enemies is
  // written once.
  TTS_EXPECT(roster.valid()) << roster.error;
  TTS_EQUAL(roster.kinds.size(), 3U);
  TTS_EQUAL(roster.kinds[2].health, 3);
  TTS_EQUAL(roster.names[2], std::string{"parasite_3"});
};

TTS_CASE("A data file cannot reach outside the game")
{
  arpg::script_host host;

  // No io, no os, no package: an edit meant to tune a number must not be able
  // to open a file or load a library.
  const arpg::enemy_catalogue roster = arpg::load_enemies(host, "return { io.open('x') }", minimum_sight);

  TTS_EXPECT_NOT(roster.valid());
};

TTS_CASE("The roster the game ships is sound")
{
  arpg::script_host host;

  // The figure the shipped file is checked against is the player's own range,
  // which is what dungeon_screen passes at runtime.
  const arpg::enemy_catalogue roster =
      arpg::load_enemies_from(host, std::filesystem::path{ARPG_ASSETS_DIR} / "data" / "enemies.lua", 264.0f);

  TTS_EXPECT(roster.valid()) << roster.error;
  TTS_EXPECT(roster.kinds.size() >= 3U);
};
