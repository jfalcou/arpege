// SPDX-License-Identifier: BSL-1.0

#include <data/enemy_data.hpp>

#include <algorithm>

namespace arpg
{

namespace
{

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
    kind.fire_interval = entry->get_or("fire_interval", 0.0f);
    kind.shot_speed = entry->get_or("shot_speed", 0.0f);
    kind.shot_radius = entry->get_or("shot_radius", 0.0f);
    kind.shot_damage = entry->get_or("shot_damage", 0);

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

    if (kind.style == attack_style::ranged && (kind.fire_interval <= 0.0f || kind.shot_speed <= 0.0f))
    {
      return refuse(where(index, name) + " shoots but has no fire_interval or shot_speed");
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
