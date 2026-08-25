// SPDX-License-Identifier: BSL-1.0

#include <data/atlas_data.hpp>

#include <data/schema.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <sstream>
#include <string_view>
#include <system_error>

namespace arpg
{

namespace
{

/// Writes the shortest run of digits that reads back as this very float.
///
/// Nine digits everywhere would also come back exact, and would put 0.100000001
/// in a file meant to be edited by hand. Shortest-round-trip gives 0.1, and
/// gives it back unchanged.
void put(std::ostringstream& out, float value)
{
  std::array<char, 32> digits{};

  const std::to_chars_result written = std::to_chars(digits.data(), digits.data() + digits.size(), value);

  if (written.ec != std::errc{})
  {
    out << value;
    return;
  }

  out << std::string_view{digits.data(), static_cast<std::size_t>(written.ptr - digits.data())};
}

bool reads_back(const sprite_atlas& atlas, std::string& error)
{
  for (const sprite_frame& frame : atlas.frames)
  {
    if (!name_is_writable(frame.name))
    {
      error = "a frame is named something that cannot be written out";
      return false;
    }
  }

  for (const sprite_animation& clip : atlas.animations)
  {
    if (!name_is_writable(clip.name))
    {
      error = "an animation is named something that cannot be written out";
      return false;
    }
  }

  return name_is_writable(atlas.image);
}

loaded_atlas read_atlas(const sol::table& described, std::string_view named)
{
  loaded_atlas out;

  const auto refuse = [&out, named](std::string message) -> loaded_atlas&
  {
    out.value = sprite_atlas{};
    out.error = std::string{named} + ": " + std::move(message);
    return out;
  };

  const std::string mismatch = schema_error(described, "this atlas", atlas_schema);

  if (!mismatch.empty())
  {
    return refuse(mismatch);
  }

  out.value.image = described.get_or("image", std::string{});

  if (out.value.image.empty())
  {
    return refuse("names no image");
  }

  const sol::optional<sol::table> frames = described["frames"];

  if (!frames)
  {
    return refuse("cuts nothing out of its image");
  }

  for (std::size_t index = 1; index <= frames->size(); ++index)
  {
    const sol::optional<sol::table> entry = (*frames)[index];

    if (!entry)
    {
      return refuse("has a frame that is not a table");
    }

    sprite_frame frame;
    frame.name = entry->get_or("name", std::string{});

    if (frame.name.empty())
    {
      return refuse("has a frame with no name");
    }

    if (out.value.find_frame(frame.name) != nullptr)
    {
      return refuse("names two frames '" + frame.name + "'");
    }

    const sol::optional<sol::table> rect = (*entry)["rect"];

    if (!rect)
    {
      return refuse("'" + frame.name + "' says nothing about where it sits");
    }

    frame.x = (*rect)[1].get_or(0);
    frame.y = (*rect)[2].get_or(0);
    frame.width = (*rect)[3].get_or(0);
    frame.height = (*rect)[4].get_or(0);

    if (frame.width <= 0 || frame.height <= 0)
    {
      return refuse("'" + frame.name + "' cuts out nothing");
    }

    const sol::optional<sol::table> origin = (*entry)["origin"];

    if (origin)
    {
      frame.origin = vec2{(*origin)[1].get_or(0.0f), (*origin)[2].get_or(0.0f)};
    }

    out.value.frames.push_back(std::move(frame));
  }

  if (out.value.frames.empty())
  {
    return refuse("cuts nothing out of its image");
  }

  const sol::optional<sol::table> animations = described["animations"];

  if (!animations)
  {
    return out;
  }

  for (std::size_t index = 1; index <= animations->size(); ++index)
  {
    const sol::optional<sol::table> entry = (*animations)[index];

    if (!entry)
    {
      return refuse("has an animation that is not a table");
    }

    sprite_animation clip;
    clip.name = entry->get_or("name", std::string{});

    if (clip.name.empty())
    {
      return refuse("has an animation with no name");
    }

    if (out.value.find_animation(clip.name) != nullptr)
    {
      return refuse("names two animations '" + clip.name + "'");
    }

    clip.loops = entry->get_or("loops", true);

    const sol::optional<sol::table> played = (*entry)["frames"];

    if (!played)
    {
      return refuse("'" + clip.name + "' plays nothing");
    }

    for (std::size_t step = 1; step <= played->size(); ++step)
    {
      const sol::optional<sol::table> pair = (*played)[step];

      if (!pair)
      {
        return refuse("'" + clip.name + "' has a step that is not a frame and a duration");
      }

      animation_frame held;
      held.frame = (*pair)[1].get_or(std::string{});
      held.seconds = (*pair)[2].get_or(0.0f);

      // Checked against the frames of this very atlas: an animation naming a
      // picture nobody cut out would draw nothing, and a typo would be found
      // by looking at the game rather than by being told.
      if (out.value.find_frame(held.frame) == nullptr)
      {
        return refuse("'" + clip.name + "' plays '" + held.frame + "', which nothing cuts out");
      }

      if (held.seconds <= 0.0f)
      {
        return refuse("'" + clip.name + "' holds a frame for no time at all");
      }

      clip.frames.push_back(std::move(held));
    }

    if (clip.frames.empty())
    {
      return refuse("'" + clip.name + "' plays nothing");
    }

    out.value.animations.push_back(std::move(clip));
  }

  return out;
}

} // namespace

bool name_is_writable(std::string_view name)
{
  if (name.empty())
  {
    return false;
  }

  return name.find_first_of("\"\\\n\r") == std::string_view::npos;
}

loaded_atlas load_atlas(script_host& host, std::string_view source, std::string_view named)
{
  const script_result script = host.run(source, named);

  if (!script.valid())
  {
    loaded_atlas out;
    out.error = script.error;
    return out;
  }

  return read_atlas(script.value, named);
}

loaded_atlas load_atlas_from(script_host& host, const std::filesystem::path& path)
{
  const script_result script = host.run_file(path);

  if (!script.valid())
  {
    loaded_atlas out;
    out.error = script.error;
    return out;
  }

  return read_atlas(script.value, path.filename().string());
}

std::string write_atlas(const sprite_atlas& atlas)
{
  std::string refused;

  if (!reads_back(atlas, refused))
  {
    return {};
  }

  std::ostringstream out;

  out << "-- SPDX-License-Identifier: BSL-1.0\n"
      << "--\n"
      << "-- Written by the sprite editor. Editing it by hand is fine; the editor\n"
      << "-- reads back what it wrote.\n"
      << "\n"
      << "return {\n"
      << "  version = " << atlas_schema << ",\n"
      << "  image = \"" << atlas.image << "\",\n"
      << "\n"
      << "  frames = {\n";

  for (const sprite_frame& frame : atlas.frames)
  {
    out << "    { name = \"" << frame.name << "\", rect = { " << frame.x << ", " << frame.y << ", " << frame.width
        << ", " << frame.height << " }, origin = { ";
    put(out, frame.origin.x);
    out << ", ";
    put(out, frame.origin.y);
    out << " } },\n";
  }

  out << "  },\n";

  if (!atlas.animations.empty())
  {
    out << "\n  animations = {\n";

    for (const sprite_animation& clip : atlas.animations)
    {
      out << "    {\n"
          << "      name = \"" << clip.name << "\",\n"
          << "      loops = " << (clip.loops ? "true" : "false") << ",\n"
          << "      frames = {\n";

      for (const animation_frame& held : clip.frames)
      {
        out << "        { \"" << held.frame << "\", ";
        put(out, held.seconds);
        out << " },\n";
      }

      out << "      },\n"
          << "    },\n";
    }

    out << "  },\n";
  }

  out << "}\n";

  return out.str();
}

} // namespace arpg
