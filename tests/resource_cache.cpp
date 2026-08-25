// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <core/resource_cache.hpp>

namespace
{

/// Stands in for a disk: the test says what exists and counts the asking.
struct fake_disk
{
  int loads = 0;
  int unloads = 0;
  std::set<std::string> exists{"sheet", "other"};

  arpg::resource_cache<int>::loader reader()
  {
    return [this](std::string_view name) -> std::optional<int>
    {
      ++loads;

      if (exists.count(std::string{name}) == 0)
      {
        return std::nullopt;
      }

      return static_cast<int>(name.size());
    };
  }

  arpg::resource_cache<int>::unloader releaser()
  {
    return [this](int&) { ++unloads; };
  }
};

} // namespace

TTS_CASE("A thing is loaded the first time and only the first time")
{
  fake_disk disk;
  arpg::resource_cache<int> cache(disk.reader());

  const int* first = cache.get("sheet");
  const int* again = cache.get("sheet");

  TTS_EXPECT(first != nullptr);
  TTS_EQUAL(disk.loads, 1);

  // The same pointer, not merely an equal value: whoever holds one is holding
  // the thing itself.
  TTS_EQUAL(first, again);
  TTS_EQUAL(cache.size(), 1U);
};

TTS_CASE("A pointer already handed out survives the cache growing")
{
  fake_disk disk;
  disk.exists = {"a", "b", "c", "d", "e", "f", "g", "h"};

  arpg::resource_cache<int> cache(disk.reader());

  const int* early = cache.get("a");
  const int held = *early;

  for (const char* name : {"b", "c", "d", "e", "f", "g", "h"})
  {
    cache.get(name);
  }

  // A cache that moved what it holds would leave everything that asked before
  // pointing at nothing, and it would work until the day it did not.
  TTS_EQUAL(*early, held);
};

TTS_CASE("What cannot be had is not asked for twice")
{
  fake_disk disk;
  arpg::resource_cache<int> cache(disk.reader());

  TTS_EXPECT(cache.get("missing") == nullptr);
  TTS_EXPECT(cache.get("missing") == nullptr);
  TTS_EXPECT(cache.get("missing") == nullptr);

  // A sheet that is not there would otherwise be looked for sixty times a
  // second, and said so sixty times a second.
  TTS_EQUAL(disk.loads, 1);
  TTS_EQUAL(cache.refused(), 1U);
};

TTS_CASE("Two names are two things")
{
  fake_disk disk;
  arpg::resource_cache<int> cache(disk.reader());

  TTS_EXPECT(cache.get("sheet") != cache.get("other"));
  TTS_EQUAL(cache.size(), 2U);
  TTS_EQUAL(disk.loads, 2);
};

TTS_CASE("Letting go tells the backend, once per thing held")
{
  fake_disk disk;

  {
    arpg::resource_cache<int> cache(disk.reader(), disk.releaser());
    cache.get("sheet");
    cache.get("other");
    cache.get("missing");
  }

  // Two held, one refused: what was never had cannot be let go of.
  TTS_EQUAL(disk.unloads, 2);
};

TTS_CASE("Letting go forgets what was refused")
{
  fake_disk disk;
  arpg::resource_cache<int> cache(disk.reader());

  TTS_EXPECT(cache.get("later") == nullptr);
  disk.exists.insert("later");

  // Still refused, since nothing has happened to say otherwise.
  TTS_EXPECT(cache.get("later") == nullptr);

  cache.clear();

  // A file put back where it belongs is looked for again.
  TTS_EXPECT(cache.get("later") != nullptr);
};

TTS_CASE("A cache with no way to load anything hands back nothing")
{
  arpg::resource_cache<int> cache;

  TTS_EXPECT(cache.get("sheet") == nullptr);
  TTS_EQUAL(cache.size(), 0U);
};

TTS_CASE("Holding is answered without going and getting")
{
  fake_disk disk;
  arpg::resource_cache<int> cache(disk.reader());

  TTS_EXPECT_NOT(cache.holds("sheet"));
  TTS_EQUAL(disk.loads, 0);

  cache.get("sheet");
  TTS_EXPECT(cache.holds("sheet"));
};
