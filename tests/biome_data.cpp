// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <data/biome_data.hpp>
#include <data/enemy_data.hpp>

#include <algorithm>

namespace
{

constexpr const char* sound_biome = R"(
  return {
    name = "the Porcelain Lazaret",
    shape = "rigid",
    rooms = { 5, 7 },
    room = { min = { 0.8, 0.8 }, max = { 1.2, 1.1 } },
    spacing = 0.3,
    fauna = { { "parasite", 5 }, { "cultist", 1 } },
  }
)";

} // namespace

TTS_CASE("A biome is read with its shape, its counts and its fauna")
{
  arpg::script_host host;
  const arpg::loaded_biomes read = arpg::load_biome(host, sound_biome, "lazaret");

  TTS_EXPECT(read.valid()) << read.error;
  TTS_EQUAL(read.all.size(), 1U);

  const arpg::biome& place = read.all.front();

  TTS_EQUAL(place.key, std::string{"lazaret"});
  TTS_EQUAL(place.name, std::string{"the Porcelain Lazaret"});
  TTS_EQUAL(place.shape, arpg::level_shape::rigid);
  TTS_EQUAL(place.rooms_min, 5);
  TTS_EQUAL(place.rooms_max, 7);
  TTS_EQUAL(place.room_max.y, 1.1f);
  TTS_EQUAL(place.fauna.size(), 2U);
  TTS_EQUAL(place.fauna[0].name, std::string{"parasite"});
  TTS_EQUAL(place.fauna[0].weight, 5);
};

TTS_CASE("Sizes are multiples of the view, turned into pixels against it")
{
  arpg::script_host host;
  const arpg::loaded_biomes read = arpg::load_biome(host, sound_biome, "lazaret");
  const arpg::level_recipe recipe = arpg::recipe_for(read.all.front(), arpg::vec2{320.0f, 180.0f});

  // A file saying a room is four fifths of a screen has to mean that on any
  // canvas, which is the whole reason it is not written in pixels.
  TTS_ULP_EQUAL(recipe.room_min.x, 256.0f, 2.0);
  TTS_ULP_EQUAL(recipe.room_min.y, 144.0f, 2.0);
  TTS_ULP_EQUAL(recipe.room_max.x, 384.0f, 2.0);
  TTS_ULP_EQUAL(recipe.room_max.y, 198.0f, 2.0);

  // Against the width alone, or the gap between two rooms would change shape
  // with the aspect of the view.
  TTS_ULP_EQUAL(recipe.spacing, 96.0f, 2.0);
};

TTS_CASE("The same biome on a bigger canvas asks for bigger rooms")
{
  arpg::script_host host;
  const arpg::loaded_biomes read = arpg::load_biome(host, sound_biome, "lazaret");

  const arpg::level_recipe small = arpg::recipe_for(read.all.front(), arpg::vec2{320.0f, 180.0f});
  const arpg::level_recipe large = arpg::recipe_for(read.all.front(), arpg::vec2{640.0f, 360.0f});

  TTS_ULP_EQUAL(large.room_min.x, small.room_min.x * 2.0f, 2.0);
  TTS_EQUAL(large.rooms_min, small.rooms_min);
};

TTS_CASE("A biome with no name to show is refused")
{
  arpg::script_host host;

  TTS_EXPECT_NOT(arpg::load_biome(host, R"(
    return { shape = "rigid", rooms = { 5, 7 },
             room = { min = { 0.8, 0.8 }, max = { 1.2, 1.1 } },
             spacing = 0.3, fauna = { { "parasite", 1 } } }
  )", "x").valid());
};

TTS_CASE("An unknown shape is refused rather than taken for the usual one")
{
  arpg::script_host host;

  TTS_EXPECT_NOT(arpg::load_biome(host, R"(
    return { name = "n", shape = "cavernous", rooms = { 5, 7 },
             room = { min = { 0.8, 0.8 }, max = { 1.2, 1.1 } },
             spacing = 0.3, fauna = { { "parasite", 1 } } }
  )", "x").valid());
};

TTS_CASE("A biome nothing lives in is refused")
{
  arpg::script_host host;

  // It would lay out rooms that clear themselves, which reads as a broken
  // level rather than as an empty one.
  TTS_EXPECT_NOT(arpg::load_biome(host, R"(
    return { name = "n", shape = "rigid", rooms = { 5, 7 },
             room = { min = { 0.8, 0.8 }, max = { 1.2, 1.1 } },
             spacing = 0.3, fauna = {} }
  )", "x").valid());
};

TTS_CASE("A count of rooms nobody can lay out is refused")
{
  arpg::script_host host;

  const char* backwards = R"(
    return { name = "n", shape = "rigid", rooms = { 9, 4 },
             room = { min = { 0.8, 0.8 }, max = { 1.2, 1.1 } },
             spacing = 0.3, fauna = { { "parasite", 1 } } }
  )";

  const char* none = R"(
    return { name = "n", shape = "rigid", rooms = { 0, 0 },
             room = { min = { 0.8, 0.8 }, max = { 1.2, 1.1 } },
             spacing = 0.3, fauna = { { "parasite", 1 } } }
  )";

  TTS_EXPECT_NOT(arpg::load_biome(host, backwards, "x").valid());
  TTS_EXPECT_NOT(arpg::load_biome(host, none, "x").valid());
};

TTS_CASE("A room smaller than nothing, or than its own minimum, is refused")
{
  arpg::script_host host;

  const char* empty = R"(
    return { name = "n", shape = "rigid", rooms = { 5, 7 },
             room = { min = { 0.0, 0.8 }, max = { 1.2, 1.1 } },
             spacing = 0.3, fauna = { { "parasite", 1 } } }
  )";

  const char* inverted = R"(
    return { name = "n", shape = "rigid", rooms = { 5, 7 },
             room = { min = { 1.4, 0.8 }, max = { 1.2, 1.1 } },
             spacing = 0.3, fauna = { { "parasite", 1 } } }
  )";

  TTS_EXPECT_NOT(arpg::load_biome(host, empty, "x").valid());
  TTS_EXPECT_NOT(arpg::load_biome(host, inverted, "x").valid());
};

TTS_CASE("A biome that forgets a whole section is refused, not defaulted")
{
  arpg::script_host host;

  // Silently handing back a size nobody wrote would make the file look
  // complete while the figures came from somewhere else.
  TTS_EXPECT_NOT(arpg::load_biome(host, R"(
    return { name = "n", shape = "rigid", rooms = { 5, 7 }, spacing = 0.3, fauna = { { "parasite", 1 } } }
  )", "x").valid());

  TTS_EXPECT_NOT(arpg::load_biome(host, R"(
    return { name = "n", shape = "rigid", rooms = { 5, 7 },
             room = { min = { 0.8, 0.8 }, max = { 1.2, 1.1 } }, fauna = { { "parasite", 1 } } }
  )", "x").valid());
};

TTS_CASE("The biomes the game ships are sound, and answer to their file names")
{
  arpg::script_host host;
  const arpg::loaded_biomes read =
      arpg::load_biomes_from(host, std::filesystem::path{ARPG_ASSETS_DIR} / "data" / "biomes");

  TTS_EXPECT(read.valid()) << read.error;
  TTS_EXPECT(read.all.size() >= 2U);

  TTS_EXPECT(read.find("lazaret") != nullptr);
  TTS_EXPECT(read.find("reef") != nullptr);
  TTS_EXPECT(read.find("nowhere") == nullptr);
};

TTS_CASE("A directory that is not there is an error like any other")
{
  arpg::script_host host;

  TTS_EXPECT_NOT(arpg::load_biomes_from(host, "no/such/place").valid());
};

TTS_CASE("A file written for another version says so, rather than complaining about a field")
{
  arpg::script_host host;
  const arpg::loaded_biomes read = arpg::load_biome(host, R"(
    return { version = 99, name = "n", shape = "rigid", rooms = { 5, 7 },
             room = { min = { 0.8, 0.8 }, max = { 1.2, 1.1 } },
             spacing = 0.3, fauna = { { "parasite", 1 } } }
  )", "x");

  // What a modder gets when their file is out of date. Without this they would
  // be told a field is missing from a file where it plainly is not.
  TTS_EXPECT_NOT(read.valid());
  TTS_EXPECT(read.error.find("version") != std::string::npos);
};

TTS_CASE("A file stating no version is taken for the first one")
{
  arpg::script_host host;

  // Which is what makes the field possible to add to a format that did not
  // have it.
  TTS_EXPECT(arpg::load_biome(host, sound_biome, "lazaret").valid());
};

TTS_CASE("Everything the shipped biomes field exists in the shipped roster")
{
  arpg::script_host host;

  const std::filesystem::path assets{ARPG_ASSETS_DIR};
  const arpg::loaded_biomes places = arpg::load_biomes_from(host, assets / "data" / "biomes");
  const arpg::enemy_catalogue roster = arpg::load_enemies_from(host, assets / "data" / "enemies.lua", 264.0f);

  TTS_EXPECT(places.valid()) << places.error;
  TTS_EXPECT(roster.valid()) << roster.error;

  // A biome fields its fauna by name, and the roster is the only place those
  // names exist. Left unchecked, a typo would quietly give a place one enemy
  // fewer than it was written with.
  for (const arpg::biome& place : places.all)
  {
    for (const arpg::biome::dweller& lives_here : place.fauna)
    {
      TTS_EXPECT(std::find(roster.names.begin(), roster.names.end(), lives_here.name) != roster.names.end())
          << place.key << " / " << lives_here.name;
    }
  }
};

TTS_CASE("A kind offered with a weight of nothing is refused")
{
  arpg::script_host host;

  // Which is a way of not listing it, and states something the file does not
  // mean.
  TTS_EXPECT_NOT(arpg::load_biome(host, R"(
    return { name = "n", shape = "rigid", rooms = { 5, 7 },
             room = { min = { 0.8, 0.8 }, max = { 1.2, 1.1 } },
             spacing = 0.3, fauna = { { "parasite", 0 } } }
  )", "x").valid());
};

TTS_CASE("A fauna written the old way, as bare names, is refused")
{
  arpg::script_host host;

  TTS_EXPECT_NOT(arpg::load_biome(host, R"(
    return { name = "n", shape = "rigid", rooms = { 5, 7 },
             room = { min = { 0.8, 0.8 }, max = { 1.2, 1.1 } },
             spacing = 0.3, fauna = { "parasite" } }
  )", "x").valid());
};
