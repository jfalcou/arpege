// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <data/atlas_data.hpp>

namespace
{

arpg::sprite_atlas sound_atlas()
{
  arpg::sprite_atlas atlas;
  atlas.image = "actors.png";

  atlas.frames.push_back(arpg::sprite_frame{"walk_0", 0, 0, 8, 12, arpg::vec2{4.0f, 12.0f}});
  atlas.frames.push_back(arpg::sprite_frame{"walk_1", 8, 0, 8, 12, arpg::vec2{4.0f, 12.0f}});
  atlas.frames.push_back(arpg::sprite_frame{"die_0", 0, 12, 8, 12, arpg::vec2{4.0f, 11.5f}});

  arpg::sprite_animation walk;
  walk.name = "walk";
  walk.loops = true;
  walk.frames.push_back(arpg::animation_frame{"walk_0", 0.12f});
  walk.frames.push_back(arpg::animation_frame{"walk_1", 0.08f});

  arpg::sprite_animation die;
  die.name = "die";
  die.loops = false;
  die.frames.push_back(arpg::animation_frame{"die_0", 0.4f});

  atlas.animations.push_back(walk);
  atlas.animations.push_back(die);

  return atlas;
}

} // namespace

TTS_CASE("An atlas written out reads back as itself")
{
  const arpg::sprite_atlas before = sound_atlas();

  arpg::script_host host;
  const arpg::loaded_atlas after = arpg::load_atlas(host, arpg::write_atlas(before), "round trip");

  TTS_EXPECT(after.valid()) << after.error;

  // The editor writes what the game reads. Were the two to drift apart, work
  // would be lost between saving it and opening it again, which is the one
  // failure a tool must not have.
  TTS_EQUAL(after.value.image, before.image);
  TTS_EQUAL(after.value.frames.size(), before.frames.size());
  TTS_EQUAL(after.value.animations.size(), before.animations.size());

  for (std::size_t index = 0; index < before.frames.size(); ++index)
  {
    const arpg::sprite_frame& a = before.frames[index];
    const arpg::sprite_frame& b = after.value.frames[index];

    TTS_EQUAL(b.name, a.name);
    TTS_EQUAL(b.x, a.x);
    TTS_EQUAL(b.y, a.y);
    TTS_EQUAL(b.width, a.width);
    TTS_EQUAL(b.height, a.height);
    TTS_EQUAL(b.origin.x, a.origin.x);
    TTS_EQUAL(b.origin.y, a.origin.y);
  }

  for (std::size_t index = 0; index < before.animations.size(); ++index)
  {
    const arpg::sprite_animation& a = before.animations[index];
    const arpg::sprite_animation& b = after.value.animations[index];

    TTS_EQUAL(b.name, a.name);
    TTS_EQUAL(b.loops, a.loops);
    TTS_EQUAL(b.frames.size(), a.frames.size());

    for (std::size_t step = 0; step < a.frames.size(); ++step)
    {
      TTS_EQUAL(b.frames[step].frame, a.frames[step].frame);

      // Written with enough digits that the float comes back the same one,
      // not merely a close one: a walk drifting out of step over a hundred
      // saves would be a mystery nobody could see in the file.
      TTS_EQUAL(b.frames[step].seconds, a.frames[step].seconds);
    }
  }
};

TTS_CASE("An atlas with nothing played still reads back")
{
  arpg::sprite_atlas bare;
  bare.image = "props.png";
  bare.frames.push_back(arpg::sprite_frame{"crate", 0, 0, 16, 16, arpg::vec2{8.0f, 16.0f}});

  arpg::script_host host;
  const arpg::loaded_atlas after = arpg::load_atlas(host, arpg::write_atlas(bare), "bare");

  TTS_EXPECT(after.valid()) << after.error;
  TTS_EQUAL(after.value.frames.size(), 1U);
  TTS_EXPECT(after.value.animations.empty());
};

TTS_CASE("A frame is found by name, and so is an animation")
{
  const arpg::sprite_atlas atlas = sound_atlas();

  TTS_EXPECT(atlas.find_frame("walk_1") != nullptr);
  TTS_EXPECT(atlas.find_frame("nothing") == nullptr);
  TTS_EXPECT(atlas.find_animation("die") != nullptr);
  TTS_EXPECT(atlas.find_animation("nothing") == nullptr);
};

TTS_CASE("An animation naming a picture nobody cut out is refused")
{
  arpg::script_host host;

  // A typo would otherwise be found by looking at the game and seeing nothing
  // drawn, rather than by being told which name is wrong.
  const arpg::loaded_atlas read = arpg::load_atlas(host, R"(
    return { image = "a.png",
             frames = { { name = "f", rect = { 0, 0, 8, 8 } } },
             animations = { { name = "walk", frames = { { "typo", 0.1 } } } } }
  )", "x");

  TTS_EXPECT_NOT(read.valid());
  TTS_EXPECT(read.error.find("typo") != std::string::npos);
};

TTS_CASE("A frame cutting out nothing is refused")
{
  arpg::script_host host;

  TTS_EXPECT_NOT(arpg::load_atlas(host, R"(
    return { image = "a.png", frames = { { name = "f", rect = { 0, 0, 0, 8 } } } }
  )", "x").valid());
};

TTS_CASE("A frame held for no time is refused")
{
  arpg::script_host host;

  // It would take no share of the animation, so it could never be seen, and
  // an animation made only of them would divide the walk by nothing.
  TTS_EXPECT_NOT(arpg::load_atlas(host, R"(
    return { image = "a.png",
             frames = { { name = "f", rect = { 0, 0, 8, 8 } } },
             animations = { { name = "walk", frames = { { "f", 0 } } } } }
  )", "x").valid());
};

TTS_CASE("Two frames of the same name are refused")
{
  arpg::script_host host;

  TTS_EXPECT_NOT(arpg::load_atlas(host, R"(
    return { image = "a.png",
             frames = { { name = "f", rect = { 0, 0, 8, 8 } },
                        { name = "f", rect = { 8, 0, 8, 8 } } } }
  )", "x").valid());
};

TTS_CASE("An atlas naming no image is refused")
{
  arpg::script_host host;

  TTS_EXPECT_NOT(arpg::load_atlas(host, R"(
    return { frames = { { name = "f", rect = { 0, 0, 8, 8 } } } }
  )", "x").valid());
};

TTS_CASE("A name that could not be read back is not written out")
{
  arpg::sprite_atlas broken;
  broken.image = "a.png";
  broken.frames.push_back(arpg::sprite_frame{"say \"hello\"", 0, 0, 8, 8, arpg::vec2{}});

  // Nothing escapes what goes between the quotes, so writing this would
  // produce a file nobody can read back. Refusing beats writing an escaper
  // for what is a mistake.
  TTS_EXPECT(arpg::write_atlas(broken).empty());
  TTS_EXPECT_NOT(arpg::name_is_writable("two\nlines"));
  TTS_EXPECT(arpg::name_is_writable("parasite_walk_0"));
};

TTS_CASE("An animation shows each of its frames in turn")
{
  const arpg::sprite_atlas atlas = sound_atlas();
  const arpg::sprite_animation& walk = *atlas.find_animation("walk");

  TTS_EQUAL(arpg::frame_at(walk, 0.0f), 0U);
  TTS_EQUAL(arpg::frame_at(walk, 0.11f), 0U);
  TTS_EQUAL(arpg::frame_at(walk, 0.13f), 1U);
  TTS_ULP_EQUAL(arpg::duration_of(walk), 0.2f, 4.0);
};

TTS_CASE("A looping animation starts over, one that does not holds its end")
{
  const arpg::sprite_atlas atlas = sound_atlas();
  const arpg::sprite_animation& walk = *atlas.find_animation("walk");
  const arpg::sprite_animation& die = *atlas.find_animation("die");

  TTS_EQUAL(arpg::frame_at(walk, 0.21f), 0U);
  TTS_EQUAL(arpg::frame_at(walk, 100.0f), arpg::frame_at(walk, 100.0f - 0.2f * 500.0f));

  // A death that started over would be a strange thing to watch.
  TTS_EQUAL(arpg::frame_at(die, 99.0f), 0U);
};

TTS_CASE("An animation nobody can play does not divide by nothing")
{
  arpg::sprite_animation empty;
  empty.name = "empty";

  arpg::sprite_animation instant;
  instant.name = "instant";
  instant.frames.push_back(arpg::animation_frame{"f", 0.0f});

  TTS_EQUAL(arpg::frame_at(empty, 1.0f), 0U);
  TTS_EQUAL(arpg::frame_at(instant, 1.0f), 0U);
  TTS_EQUAL(arpg::duration_of(empty), 0.0f);
};

TTS_CASE("Time before the beginning shows the first frame")
{
  const arpg::sprite_atlas atlas = sound_atlas();

  // An interpolated clock can hand back a hair below zero on the step a clip
  // starts, and a negative share of an animation is not a frame.
  const arpg::sprite_animation& walk = *atlas.find_animation("walk");
  TTS_EQUAL(arpg::frame_at(walk, -0.5f), 0U);
};

TTS_CASE("A sheet on a grid cuts into whole cells, row by row")
{
  const std::vector<arpg::sprite_frame> cut = arpg::slice_grid(32, 16, arpg::grid_slice{.cell_width = 8, .cell_height = 8});

  TTS_EQUAL(cut.size(), 8U);
  TTS_EQUAL(cut[0].x, 0);
  TTS_EQUAL(cut[0].y, 0);
  TTS_EQUAL(cut[3].x, 24);
  TTS_EQUAL(cut[3].y, 0);
  TTS_EQUAL(cut[4].x, 0);
  TTS_EQUAL(cut[4].y, 8);
  TTS_EQUAL(cut[7].name, std::string{"frame_7"});
};

TTS_CASE("A column that does not fit yields no half frame")
{
  // A sheet 30 wide holds three cells of eight and six pixels nobody asked
  // for. Half a picture is never what was meant.
  const std::vector<arpg::sprite_frame> cut = arpg::slice_grid(30, 8, arpg::grid_slice{.cell_width = 8, .cell_height = 8});

  TTS_EQUAL(cut.size(), 3U);
  TTS_EQUAL(cut.back().x + cut.back().width, 24);
};

TTS_CASE("Margin and spacing are left alone")
{
  const std::vector<arpg::sprite_frame> cut =
      arpg::slice_grid(40, 20, arpg::grid_slice{.cell_width = 8, .cell_height = 8, .margin = 2, .spacing = 4});

  TTS_EQUAL(cut[0].x, 2);
  TTS_EQUAL(cut[0].y, 2);
  TTS_EQUAL(cut[1].x, 14);
};

TTS_CASE("The origin of a cut frame stands at its feet")
{
  const std::vector<arpg::sprite_frame> cut =
      arpg::slice_grid(16, 16, arpg::grid_slice{.cell_width = 16, .cell_height = 16});

  // The middle of the width and the bottom of the height, which is what a body
  // seen from above stands on.
  TTS_EQUAL(cut[0].origin.x, 8.0f);
  TTS_EQUAL(cut[0].origin.y, 16.0f);
};

TTS_CASE("A cut nobody could make yields nothing")
{
  TTS_EXPECT(arpg::slice_grid(32, 32, arpg::grid_slice{.cell_width = 0}).empty());
  TTS_EXPECT(arpg::slice_grid(32, 32, arpg::grid_slice{.cell_height = -4}).empty());
  TTS_EXPECT(arpg::slice_grid(4, 4, arpg::grid_slice{.cell_width = 8, .cell_height = 8}).empty());
};

TTS_CASE("A duration is written as short as it can be and still read back")
{
  arpg::sprite_atlas atlas;
  atlas.image = "a.png";
  atlas.frames.push_back(arpg::sprite_frame{"f", 0, 0, 8, 8, arpg::vec2{4.0f, 8.0f}});

  arpg::sprite_animation clip;
  clip.name = "walk";
  clip.frames.push_back(arpg::animation_frame{"f", 0.1f});
  atlas.animations.push_back(clip);

  const std::string written = arpg::write_atlas(atlas);

  // Nine digits everywhere also comes back exact, and puts 0.100000001 in a
  // file meant to be read and edited by a person.
  TTS_EXPECT(written.find("0.1 ") != std::string::npos) << written;
  TTS_EXPECT(written.find("0.100000") == std::string::npos);

  arpg::script_host host;
  const arpg::loaded_atlas back = arpg::load_atlas(host, written, "x");

  TTS_EXPECT(back.valid()) << back.error;
  TTS_EQUAL(back.value.animations[0].frames[0].seconds, 0.1f);
};
