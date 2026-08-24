// SPDX-License-Identifier: BSL-1.0

#include <data/enemy_data.hpp>

#include <algorithm>

namespace arpg
{

namespace
{

/// What a volley may hold before the figure is taken for a mistake.
constexpr int bullets_per_volley_limit = 256;

/// Points at the offending entry the way the file reads: one-based, and by
/// name when it has one.
std::string where(std::size_t index, const std::string& name)
{
  const std::string position = "entry " + std::to_string(index + 1);
  return name.empty() ? position : position + " (" + name + ")";
}

enemy_catalogue read_enemies(const sol::table& roster, float minimum_sight)
{
  enemy_catalogue out;

  const auto refuse = [&out](std::string message) -> enemy_catalogue&
  {
    out.kinds.clear();
    out.names.clear();
    out.error = std::move(message);
    return out;
  };

  const std::size_t count = roster.size();

  for (std::size_t index = 0; index < count; ++index)
  {
    const sol::optional<sol::table> entry = roster[index + 1];

    if (!entry)
    {
      return refuse(where(index, {}) + " is not a table");
    }

    const std::string name = entry->get_or("name", std::string{});

    if (name.empty())
    {
      return refuse(where(index, {}) + " has no name");
    }

    if (std::find(out.names.begin(), out.names.end(), name) != out.names.end())
    {
      return refuse(where(index, name) + " repeats a name already taken");
    }

    const std::string style = entry->get_or("style", std::string{"melee"});

    if (style != "melee" && style != "ranged")
    {
      return refuse(where(index, name) + " has an unknown style '" + style + "'");
    }

    enemy_archetype kind{};
    kind.cost = entry->get_or("cost", 0);
    kind.health = entry->get_or("health", 0);
    kind.speed = entry->get_or("speed", 0.0f);
    kind.radius = entry->get_or("radius", 0.0f);
    kind.touch = entry->get_or("touch", 0);
    kind.sight = entry->get_or("sight", 0.0f);
    kind.reach = entry->get_or("reach", 0.0f);
    kind.style = (style == "ranged") ? attack_style::ranged : attack_style::melee;
    const sol::optional<sol::table> shots = (*entry)["shots"];

    if (shots)
    {
      const std::string mode = shots->get_or("aim", std::string{"aimed"});

      if (mode != "aimed" && mode != "fixed")
      {
        return refuse(where(index, name) + " aims in an unknown way '" + mode + "'");
      }

      kind.shots.aim = (mode == "fixed") ? aim_mode::fixed : aim_mode::aimed;
      kind.shots.bullets = shots->get_or("bullets", 1);
      kind.shots.arc = shots->get_or("arc", 0.0f);
      kind.shots.spin = shots->get_or("spin", 0.0f);
      kind.shots.interval = shots->get_or("interval", 0.0f);
      kind.shots.speed = shots->get_or("speed", 0.0f);
      kind.shots.radius = shots->get_or("radius", 2.0f);
      kind.shots.damage = shots->get_or("damage", 1);
      kind.shots.life = shots->get_or("life", 4.0f);
    }

    // Composing a wave buys archetypes until the budget runs out, so one that
    // costs nothing is bought forever.
    if (kind.cost <= 0)
    {
      return refuse(where(index, name) + " must cost more than nothing");
    }

    if (kind.health <= 0)
    {
      return refuse(where(index, name) + " must have health");
    }

    if (kind.radius <= 0.0f)
    {
      return refuse(where(index, name) + " must have a radius");
    }

    if (kind.reach <= 0.0f)
    {
      return refuse(where(index, name) + " must have a reach");
    }

    if (kind.sight < minimum_sight)
    {
      return refuse(where(index, name) + " wakes at " + std::to_string(static_cast<int>(kind.sight)) +
                    ", closer than the " + std::to_string(static_cast<int>(minimum_sight)) +
                    " the player can strike from");
    }

    if (kind.style == attack_style::ranged)
    {
      if (!shots)
      {
        return refuse(where(index, name) + " shoots but describes no shots");
      }

      if (kind.shots.interval <= 0.0f || kind.shots.speed <= 0.0f || kind.shots.bullets <= 0)
      {
        return refuse(where(index, name) + " shoots but has no interval, speed or bullets");
      }

      // A typo in a hot-reloaded file must not put ten thousand entities in
      // the room at once: what would follow is a freeze rather than a
      // readable mistake.
      if (kind.shots.bullets > bullets_per_volley_limit)
      {
        return refuse(where(index, name) + " fires more than " + std::to_string(bullets_per_volley_limit) +
                      " bullets in one volley");
      }
    }

    out.kinds.push_back(kind);
    out.names.push_back(name);
  }

  if (out.kinds.empty())
  {
    out.error = "the roster is empty";
  }

  return out;
}

} // namespace

enemy_catalogue load_enemies(script_host& host, std::string_view source, float minimum_sight)
{
  const script_result script = host.run(source, "enemies");

  if (!script.valid())
  {
    enemy_catalogue out;
    out.error = script.error;
    return out;
  }

  return read_enemies(script.value, minimum_sight);
}

enemy_catalogue load_enemies_from(script_host& host, const std::filesystem::path& path, float minimum_sight)
{
  const script_result script = host.run_file(path);

  if (!script.valid())
  {
    enemy_catalogue out;
    out.error = script.error;
    return out;
  }

  return read_enemies(script.value, minimum_sight);
}

} // namespace arpg
