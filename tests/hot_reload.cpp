// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <data/hot_reload.hpp>

namespace
{

/// Stands in for the filesystem: the test says what the file looks like and
/// counts how often it was asked, which the real one cannot be made to do.
struct fake_disk
{
  std::optional<arpg::file_stamp> stamp = arpg::file_stamp{};
  int reads = 0;

  arpg::stamp_reader reader()
  {
    return [this](const std::filesystem::path&) -> std::optional<arpg::file_stamp>
    {
      ++reads;
      return stamp;
    };
  }

  void rewrite(std::uintmax_t size) { stamp = arpg::file_stamp{{}, size}; }
};

constexpr float step = 1.0f / 60.0f;

} // namespace

TTS_CASE("A file nobody touched never reports a change")
{
  fake_disk disk;
  arpg::file_watch watch("roster.lua", 1.0f, disk.reader());

  for (int i = 0; i < 600; ++i)
  {
    TTS_EXPECT_NOT(watch.poll(step));
  }
};

TTS_CASE("A rewritten file is reported once")
{
  fake_disk disk;
  arpg::file_watch watch("roster.lua", 1.0f, disk.reader());

  disk.rewrite(128);

  bool reported = false;
  for (int i = 0; i < 120 && !reported; ++i)
  {
    reported = watch.poll(step);
  }

  TTS_EXPECT(reported);

  // The new version is now the one remembered, so it is not reported again.
  for (int i = 0; i < 120; ++i)
  {
    TTS_EXPECT_NOT(watch.poll(step));
  }
};

TTS_CASE("The filesystem is asked on the interval, not every frame")
{
  fake_disk disk;
  arpg::file_watch watch("roster.lua", 1.0f, disk.reader());

  const int at_start = disk.reads;

  for (int i = 0; i < 600; ++i)
  {
    watch.poll(step);
  }

  // Ten seconds of frames. What matters is that the count follows the seconds
  // rather than the frames; pinning it to the exact figure would be asserting
  // where the rounding of a float accumulator lands.
  const int asked = disk.reads - at_start;
  TTS_EXPECT(asked >= 9 && asked <= 11);
};

TTS_CASE("The interval is exactly what was asked for")
{
  fake_disk disk;
  arpg::file_watch watch("roster.lua", 2.0f, disk.reader());

  const int at_start = disk.reads;

  watch.poll(1.0f);
  TTS_EQUAL(disk.reads - at_start, 0);

  watch.poll(1.0f);
  TTS_EQUAL(disk.reads - at_start, 1);
};

TTS_CASE("Several edits inside one interval read as one change")
{
  fake_disk disk;
  arpg::file_watch watch("roster.lua", 1.0f, disk.reader());

  disk.rewrite(1);
  watch.poll(0.2f);
  disk.rewrite(2);
  watch.poll(0.2f);
  disk.rewrite(3);

  TTS_EXPECT(watch.poll(1.0f));
  TTS_EXPECT_NOT(watch.poll(1.0f));
};

TTS_CASE("A file that cannot be reached is no news")
{
  fake_disk disk;
  arpg::file_watch watch("roster.lua", 1.0f, disk.reader());

  // What an editor writing the file looks like from here. Reporting it would
  // reload a file in the middle of being written.
  disk.stamp = std::nullopt;
  TTS_EXPECT_NOT(watch.poll(1.0f));

  disk.rewrite(64);
  TTS_EXPECT(watch.poll(1.0f));
};

TTS_CASE("A file that was already missing is reported once it appears")
{
  fake_disk disk;
  disk.stamp = std::nullopt;

  arpg::file_watch watch("roster.lua", 1.0f, disk.reader());

  disk.rewrite(32);

  TTS_EXPECT(watch.poll(1.0f));
};

TTS_CASE("A default watch polls nothing")
{
  arpg::file_watch watch;

  TTS_EXPECT_NOT(watch.poll(10.0f));
};

namespace
{

/// Stands in for a directory: the test says what it holds.
struct fake_directory
{
  arpg::directory_listing holds{{"lazaret.lua", arpg::file_stamp{}}};
  int reads = 0;

  arpg::directory_reader reader()
  {
    return [this](const std::filesystem::path&)
    {
      ++reads;
      return holds;
    };
  }
};

} // namespace

TTS_CASE("A directory nobody touched never reports a change")
{
  fake_directory disk;
  arpg::directory_watch watch("biomes", 1.0f, disk.reader());

  for (int i = 0; i < 600; ++i)
  {
    TTS_EXPECT_NOT(watch.poll(step));
  }
};

TTS_CASE("A file dropped in beside the others is a change")
{
  fake_directory disk;
  arpg::directory_watch watch("biomes", 1.0f, disk.reader());

  // The whole point of one file per biome: adding one has to take effect
  // without anything else being touched.
  disk.holds.emplace_back("reef.lua", arpg::file_stamp{});

  TTS_EXPECT(watch.poll(1.0f));
  TTS_EXPECT_NOT(watch.poll(1.0f));
};

TTS_CASE("A file taken away is a change too")
{
  fake_directory disk;
  disk.holds.emplace_back("reef.lua", arpg::file_stamp{});

  arpg::directory_watch watch("biomes", 1.0f, disk.reader());
  disk.holds.pop_back();

  TTS_EXPECT(watch.poll(1.0f));
};

TTS_CASE("A file rewritten in place is a change")
{
  fake_directory disk;
  arpg::directory_watch watch("biomes", 1.0f, disk.reader());

  disk.holds[0].second.size = 4096;

  TTS_EXPECT(watch.poll(1.0f));
};

TTS_CASE("A directory that reads as empty is no news")
{
  fake_directory disk;
  arpg::directory_watch watch("biomes", 1.0f, disk.reader());

  // What a directory being written to looks like from here. Reporting it would
  // reload a world with no biome left in it.
  disk.holds.clear();
  TTS_EXPECT_NOT(watch.poll(1.0f));

  disk.holds.emplace_back("lazaret.lua", arpg::file_stamp{});
  disk.holds[0].second.size = 128;
  TTS_EXPECT(watch.poll(1.0f));
};

TTS_CASE("A directory is listed on the interval, not every frame")
{
  fake_directory disk;
  arpg::directory_watch watch("biomes", 1.0f, disk.reader());

  const int at_start = disk.reads;

  for (int i = 0; i < 600; ++i)
  {
    watch.poll(step);
  }

  const int asked = disk.reads - at_start;
  TTS_EXPECT(asked >= 9 && asked <= 11);
};

TTS_CASE("A default directory watch polls nothing")
{
  arpg::directory_watch watch;

  TTS_EXPECT_NOT(watch.poll(10.0f));
};
