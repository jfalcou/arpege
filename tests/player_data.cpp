// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <data/player_data.hpp>

namespace
{

constexpr const char* sound_player = R"(
  return {
    health = 3, speed = 70, focus_speed = 30, hitbox = 2, mercy = 0.8,
    gun = { interval = 0.12, speed = 220, radius = 1.5, life = 1.2, damage = 1 },
    dash = { speed = 260, duration = 0.14, cooldown = 0.55, mercy = 0.14 },
  }
)";

} // namespace

TTS_CASE("A profile is read with its gun and its dash")
{
  arpg::script_host host;
  const arpg::loaded_player hero = arpg::load_player(host, sound_player);

  TTS_EXPECT(hero.valid()) << hero.error;
  TTS_EQUAL(hero.value.health, 3);
  TTS_EQUAL(hero.value.speed, 70.0f);
  TTS_EQUAL(hero.value.hitbox, 2.0f);
  TTS_EQUAL(hero.value.bullet_speed, 220.0f);
  TTS_EQUAL(hero.value.dash.speed, 260.0f);
  TTS_EQUAL(hero.value.dash.mercy, 0.14f);
};

TTS_CASE("The range of the gun is what its life buys at its speed")
{
  arpg::script_host host;
  const arpg::loaded_player hero = arpg::load_player(host, sound_player);

  // This is the figure every archetype is checked against, so it has to fall
  // out of the same file rather than be restated somewhere else.
  TTS_ULP_EQUAL(hero.value.range(), 220.0f * 1.2f, 2.0);
};

TTS_CASE("A player with no health is refused")
{
  arpg::script_host host;

  TTS_EXPECT_NOT(arpg::load_player(host, R"(
    return { health = 0, speed = 70, focus_speed = 30, hitbox = 2,
             gun = { interval = 0.12, speed = 220, life = 1.2 },
             dash = { speed = 260, duration = 0.14 } }
  )").valid());
};

TTS_CASE("A player who cannot move is refused")
{
  arpg::script_host host;

  TTS_EXPECT_NOT(arpg::load_player(host, R"(
    return { health = 3, speed = 0, focus_speed = 30, hitbox = 2,
             gun = { interval = 0.12, speed = 220, life = 1.2 },
             dash = { speed = 260, duration = 0.14 } }
  )").valid());
};

TTS_CASE("A gun with no life is refused")
{
  arpg::script_host host;

  // A shot that never expires crosses the room and kills what the player
  // cannot even see.
  TTS_EXPECT_NOT(arpg::load_player(host, R"(
    return { health = 3, speed = 70, focus_speed = 30, hitbox = 2,
             gun = { interval = 0.12, speed = 220 },
             dash = { speed = 260, duration = 0.14 } }
  )").valid());
};

TTS_CASE("A missing dash is refused rather than defaulted")
{
  arpg::script_host host;

  // Silently handing back a dash nobody described would make the file look
  // complete while the figures came from somewhere else.
  TTS_EXPECT_NOT(arpg::load_player(host, R"(
    return { health = 3, speed = 70, focus_speed = 30, hitbox = 2,
             gun = { interval = 0.12, speed = 220, life = 1.2 } }
  )").valid());
};

TTS_CASE("A refused profile hands back nothing half read")
{
  arpg::script_host host;
  const arpg::loaded_player hero = arpg::load_player(host, R"(
    return { health = 99, speed = 0, focus_speed = 30, hitbox = 2,
             gun = { interval = 0.12, speed = 220, life = 1.2 },
             dash = { speed = 260, duration = 0.14 } }
  )");

  TTS_EXPECT_NOT(hero.valid());
  TTS_EQUAL(hero.value.health, arpg::player_profile{}.health);
};

TTS_CASE("A file that is not there is an error like any other")
{
  arpg::script_host host;

  TTS_EXPECT_NOT(arpg::load_player_from(host, "no/such/player.lua").valid());
};

TTS_CASE("The profile the game ships is sound")
{
  arpg::script_host host;
  const arpg::loaded_player hero =
      arpg::load_player_from(host, std::filesystem::path{ARPG_ASSETS_DIR} / "data" / "player.lua");

  TTS_EXPECT(hero.valid()) << hero.error;
};
