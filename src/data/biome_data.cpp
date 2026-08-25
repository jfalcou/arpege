// SPDX-License-Identifier: BSL-1.0

#include <data/biome_data.hpp>

#include <data/schema.hpp>

#include <algorithm>
#include <system_error>

namespace arpg
{

namespace
{

/// Reads a pair of numbers written as a Lua array, which is how a size is
/// spelt in these files.
bool read_pair(const sol::table& from, const char* field, vec2& into)
{
  const sol::optional<sol::table> pair = from[field];

  if (!pair)
  {
    return false;
  }

  const sol::optional<float> x = (*pair)[1];
  const sol::optional<float> y = (*pair)[2];

  if (!x || !y)
  {
    return false;
  }

  into = vec2{*x, *y};
  return true;
}

loaded_biomes read_biome(const sol::table& described, std::string_view key)
{
  loaded_biomes out;
  biome place;
  place.key = std::string{key};

  const auto refuse = [&out, key](std::string message) -> loaded_biomes&
  {
    out.all.clear();
    out.error = std::string{key} + ": " + std::move(message);
    return out;
  };

  const std::string mismatch = schema_error(described, "this biome", biome_schema);

  if (!mismatch.empty())
  {
    return refuse(mismatch);
  }

  place.name = described.get_or("name", std::string{});

  if (place.name.empty())
  {
    return refuse("no name to show");
  }

  const std::string shape = described.get_or("shape", std::string{});

  if (shape != "rigid" && shape != "organic")
  {
    return refuse("unknown shape '" + shape + "'");
  }

  place.shape = (shape == "rigid") ? level_shape::rigid : level_shape::organic;

  const sol::optional<sol::table> rooms = described["rooms"];

  if (!rooms)
  {
    return refuse("says nothing about how many rooms a level holds");
  }

  place.rooms_min = (*rooms)[1].get_or(0);
  place.rooms_max = (*rooms)[2].get_or(0);

  if (place.rooms_min < 1 || place.rooms_max < place.rooms_min)
  {
    return refuse("asks for a number of rooms nobody can lay out");
  }

  const sol::optional<sol::table> room = described["room"];

  if (!room || !read_pair(*room, "min", place.room_min) || !read_pair(*room, "max", place.room_max))
  {
    return refuse("says nothing about how big a room is");
  }

  if (place.room_min.x <= 0.0f || place.room_min.y <= 0.0f || place.room_max.x < place.room_min.x ||
      place.room_max.y < place.room_min.y)
  {
    return refuse("asks for a room of no size, or of a size below its own minimum");
  }

  place.spacing = described.get_or("spacing", -1.0f);

  if (place.spacing < 0.0f)
  {
    return refuse("says nothing about the ground left between two rooms");
  }

  const sol::optional<sol::table> fauna = described["fauna"];

  if (fauna)
  {
    for (std::size_t index = 1; index <= fauna->size(); ++index)
    {
      const sol::optional<sol::table> pair = (*fauna)[index];

      if (!pair)
      {
        return refuse("lists something in its fauna that is not a name and a weight");
      }

      const std::string named = (*pair)[1].get_or(std::string{});
      const int weight = (*pair)[2].get_or(0);

      if (named.empty())
      {
        return refuse("names something in its fauna that is not a name");
      }

      if (weight <= 0)
      {
        return refuse("offers '" + named + "' with a weight of nothing, which is a way of not listing it");
      }

      place.fauna.push_back(biome::dweller{named, weight});
    }
  }

  if (place.fauna.empty())
  {
    // A place with nothing living in it would lay out rooms that clear
    // themselves, which reads as a broken level rather than as an empty one.
    return refuse("has no fauna");
  }

  out.all.push_back(std::move(place));
  return out;
}

} // namespace

const biome* loaded_biomes::find(std::string_view key) const
{
  const auto found = std::find_if(all.begin(), all.end(), [key](const biome& place) { return place.key == key; });

  return (found == all.end()) ? nullptr : &(*found);
}

loaded_biomes load_biome(script_host& host, std::string_view source, std::string_view key)
{
  const script_result script = host.run(source, key);

  if (!script.valid())
  {
    loaded_biomes out;
    out.error = script.error;
    return out;
  }

  return read_biome(script.value, key);
}

loaded_biomes load_biomes_from(script_host& host, const std::filesystem::path& directory)
{
  loaded_biomes out;

  std::error_code failure;
  std::vector<std::filesystem::path> files;

  for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directory, failure))
  {
    if (entry.is_regular_file() && entry.path().extension() == ".lua")
    {
      files.push_back(entry.path());
    }
  }

  if (failure)
  {
    out.error = "cannot read " + directory.string();
    return out;
  }

  if (files.empty())
  {
    out.error = "no biome in " + directory.string();
    return out;
  }

  std::sort(files.begin(), files.end());

  for (const std::filesystem::path& file : files)
  {
    const script_result script = host.run_file(file);

    if (!script.valid())
    {
      out.all.clear();
      out.error = script.error;
      return out;
    }

    loaded_biomes one = read_biome(script.value, file.stem().string());

    if (!one.valid())
    {
      out.all.clear();
      out.error = std::move(one.error);
      return out;
    }

    out.all.push_back(std::move(one.all.front()));
  }

  return out;
}

level_recipe recipe_for(const biome& place, vec2 screen)
{
  return level_recipe{.shape = place.shape,
                      .rooms_min = place.rooms_min,
                      .rooms_max = place.rooms_max,
                      .room_min = vec2{place.room_min.x * screen.x, place.room_min.y * screen.y},
                      .room_max = vec2{place.room_max.x * screen.x, place.room_max.y * screen.y},
                      // Measured against the width alone, so the gap between two
                      // rooms does not change shape with the aspect of the view.
                      .spacing = place.spacing * screen.x};
}

} // namespace arpg
