// SPDX-License-Identifier: BSL-1.0

#include <data/player_data.hpp>

namespace arpg
{

namespace
{

loaded_player read_player(const sol::table& described)
{
  loaded_player out;
  player_profile& profile = out.value;

  profile.health = described.get_or("health", 0);
  profile.speed = described.get_or("speed", 0.0f);
  profile.focus_speed = described.get_or("focus_speed", 0.0f);
  profile.hitbox = described.get_or("hitbox", 0.0f);
  profile.mercy = described.get_or("mercy", 0.0f);

  const sol::optional<sol::table> gun = described["gun"];

  if (gun)
  {
    profile.fire_interval = gun->get_or("interval", 0.0f);
    profile.bullet_speed = gun->get_or("speed", 0.0f);
    profile.bullet_radius = gun->get_or("radius", 0.0f);
    profile.bullet_life = gun->get_or("life", 0.0f);
    profile.bullet_damage = gun->get_or("damage", 0);
  }

  const sol::optional<sol::table> dash = described["dash"];

  if (dash)
  {
    profile.dash.speed = dash->get_or("speed", 0.0f);
    profile.dash.duration = dash->get_or("duration", 0.0f);
    profile.dash.cooldown = dash->get_or("cooldown", 0.0f);
    profile.dash.mercy = dash->get_or("mercy", 0.0f);
  }

  const auto refuse = [&out](std::string message) -> loaded_player&
  {
    out.value = player_profile{};
    out.error = std::move(message);
    return out;
  };

  if (profile.health <= 0)
  {
    return refuse("the player must have health");
  }

  if (profile.speed <= 0.0f || profile.focus_speed <= 0.0f)
  {
    return refuse("the player must be able to move");
  }

  if (profile.hitbox <= 0.0f)
  {
    return refuse("the player must have a hitbox");
  }

  if (!gun)
  {
    return refuse("the player describes no gun");
  }

  if (profile.fire_interval <= 0.0f || profile.bullet_speed <= 0.0f || profile.bullet_life <= 0.0f)
  {
    // A shot that never expires would cross the room and kill what the player
    // cannot see, and one fired every step would be a solid beam.
    return refuse("the gun needs an interval, a speed and a life");
  }

  if (!dash)
  {
    return refuse("the player describes no dash");
  }

  if (profile.dash.speed <= 0.0f || profile.dash.duration <= 0.0f)
  {
    return refuse("the dash needs a speed and a duration");
  }

  return out;
}

} // namespace

loaded_player load_player(script_host& host, std::string_view source)
{
  const script_result script = host.run(source, "player");

  if (!script.valid())
  {
    loaded_player out;
    out.error = script.error;
    return out;
  }

  return read_player(script.value);
}

loaded_player load_player_from(script_host& host, const std::filesystem::path& path)
{
  const script_result script = host.run_file(path);

  if (!script.valid())
  {
    loaded_player out;
    out.error = script.error;
    return out;
  }

  return read_player(script.value);
}

} // namespace arpg
