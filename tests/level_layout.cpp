// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <world/level_layout.hpp>

#include <algorithm>

namespace
{

constexpr arpg::level_recipe rigid{
    .shape = arpg::level_shape::rigid, .rooms = 8, .room_min = {200.0f, 200.0f}, .room_max = {420.0f, 420.0f}, .spacing = 48.0f};

constexpr arpg::level_recipe organic{.shape = arpg::level_shape::organic,
                                     .rooms = 8,
                                     .room_min = {200.0f, 200.0f},
                                     .room_max = {420.0f, 420.0f},
                                     .spacing = 48.0f,
                                     .service_scale = 0.5f};

bool overlap(const arpg::viewport_rect& a, const arpg::viewport_rect& b)
{
  return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height && b.y < a.y + a.height;
}

/// Every pair of rooms, since two arenas sharing ground would let a fight spill
/// into a room the player has not entered.
bool any_overlap(const arpg::level_layout& layout)
{
  for (std::size_t i = 0; i < layout.rooms.size(); ++i)
  {
    for (std::size_t j = i + 1; j < layout.rooms.size(); ++j)
    {
      if (overlap(layout.rooms[i].bounds, layout.rooms[j].bounds))
      {
        return true;
      }
    }
  }

  return false;
}

std::size_t count_of(const arpg::level_layout& layout, arpg::room_role role)
{
  return static_cast<std::size_t>(
      std::count_if(layout.rooms.begin(), layout.rooms.end(), [role](const arpg::level_room& r)
                    { return r.role == role; }));
}

} // namespace

TTS_CASE("Every room can be reached from the entrance")
{
  // Across many seeds rather than one: a generator that shuts the player out
  // of a room does it rarely, which is exactly what one example would miss.
  for (std::uint64_t seed = 1; seed <= 200; ++seed)
  {
    arpg::rng generator(seed);
    TTS_EXPECT(arpg::fully_connected(arpg::generate_level(rigid, generator)));

    arpg::rng other(seed);
    TTS_EXPECT(arpg::fully_connected(arpg::generate_level(organic, other)));
  }
};

TTS_CASE("No two rooms share ground")
{
  for (std::uint64_t seed = 1; seed <= 200; ++seed)
  {
    arpg::rng generator(seed);
    TTS_EXPECT_NOT(any_overlap(arpg::generate_level(rigid, generator)));

    arpg::rng other(seed);
    TTS_EXPECT_NOT(any_overlap(arpg::generate_level(organic, other)));
  }
};

TTS_CASE("A level has one entrance and one boss, and they are not the same room")
{
  for (std::uint64_t seed = 1; seed <= 100; ++seed)
  {
    arpg::rng generator(seed);
    const arpg::level_layout level = arpg::generate_level(organic, generator);

    TTS_EQUAL(count_of(level, arpg::room_role::start), 1U);
    TTS_EQUAL(count_of(level, arpg::room_role::boss), 1U);
    TTS_EXPECT(level.start != level.boss);
  }
};

TTS_CASE("The boss is as far from the entrance as the level allows")
{
  arpg::rng generator(7);
  const arpg::level_layout level = arpg::generate_level(organic, generator);

  // Reaching it must mean having crossed the level rather than having turned a
  // corner, so nothing may sit further away.
  const auto steps_to = [&level](std::size_t room)
  {
    std::vector<std::size_t> frontier{level.start};
    std::vector<bool> seen(level.rooms.size(), false);
    seen[level.start] = true;
    std::size_t distance = 0;

    while (!frontier.empty())
    {
      if (std::find(frontier.begin(), frontier.end(), room) != frontier.end())
      {
        return distance;
      }

      std::vector<std::size_t> next;
      for (const std::size_t here : frontier)
      {
        for (const std::size_t neighbour : arpg::neighbours_of(level, here))
        {
          if (!seen[neighbour])
          {
            seen[neighbour] = true;
            next.push_back(neighbour);
          }
        }
      }

      frontier = next;
      ++distance;
    }

    return std::size_t{0};
  };

  const std::size_t to_boss = steps_to(level.boss);

  for (std::size_t index = 0; index < level.rooms.size(); ++index)
  {
    TTS_EXPECT(steps_to(index) <= to_boss);
  }
};

TTS_CASE("A level always offers somewhere to send loot back")
{
  for (std::uint64_t seed = 1; seed <= 100; ++seed)
  {
    arpg::rng generator(seed);
    const arpg::level_layout level = arpg::generate_level(rigid, generator);

    // A station that only existed on some layouts would make the loop it
    // drives come and go.
    TTS_EQUAL(count_of(level, arpg::room_role::station), 1U);
  }
};

TTS_CASE("The same seed lays out the same level")
{
  arpg::rng once(1234);
  arpg::rng twice(1234);

  const arpg::level_layout a = arpg::generate_level(organic, once);
  const arpg::level_layout b = arpg::generate_level(organic, twice);

  TTS_EQUAL(a.rooms.size(), b.rooms.size());
  TTS_EQUAL(a.start, b.start);
  TTS_EQUAL(a.boss, b.boss);

  for (std::size_t index = 0; index < a.rooms.size(); ++index)
  {
    TTS_EQUAL(a.rooms[index].bounds.x, b.rooms[index].bounds.x);
    TTS_EQUAL(a.rooms[index].bounds.y, b.rooms[index].bounds.y);
    TTS_EQUAL(a.rooms[index].bounds.width, b.rooms[index].bounds.width);
    TTS_EQUAL(static_cast<int>(a.rooms[index].role), static_cast<int>(b.rooms[index].role));
  }
};

TTS_CASE("Two seeds lay out two levels")
{
  arpg::rng one(1);
  arpg::rng two(2);

  const arpg::level_layout a = arpg::generate_level(organic, one);
  const arpg::level_layout b = arpg::generate_level(organic, two);

  const bool identical =
      a.rooms.size() == b.rooms.size() &&
      std::equal(a.rooms.begin(), a.rooms.end(), b.rooms.begin(), [](const arpg::level_room& x, const arpg::level_room& y)
                 { return x.bounds.x == y.bounds.x && x.bounds.y == y.bounds.y; });

  TTS_EXPECT_NOT(identical);
};

TTS_CASE("The room that holds no fight is not built like an arena")
{
  for (std::uint64_t seed = 1; seed <= 100; ++seed)
  {
    arpg::rng generator(seed);
    const arpg::level_layout level = arpg::generate_level(organic, generator);

    for (const arpg::level_room& room : level.rooms)
    {
      if (room.role != arpg::room_role::station)
      {
        continue;
      }

      // A desk and the clerk behind it. Giving it the size of an arena would
      // promise a fight that never comes, and taking a fraction of whatever
      // plot it landed on would still leave a hall on a large one.
      TTS_EXPECT(room.bounds.width <= organic.room_min.x * organic.service_scale);
      TTS_EXPECT(room.bounds.height <= organic.room_min.y * organic.service_scale);
    }
  }
};

TTS_CASE("Shrinking the station leaves it clear of its neighbours")
{
  // It is taken in about its own centre, inside ground it already held, so a
  // layout that was sound before stays sound.
  for (std::uint64_t seed = 1; seed <= 100; ++seed)
  {
    arpg::rng generator(seed);
    TTS_EXPECT_NOT(any_overlap(arpg::generate_level(organic, generator)));
  }
};

TTS_CASE("Every room is the size the recipe asked for")
{
  // Both shapes: one draws its sizes and trivially honours them, the other
  // carves them out of a box and has to be held to the same promise.
  for (const arpg::level_recipe& recipe : {rigid, organic})
  {
    for (std::uint64_t seed = 1; seed <= 50; ++seed)
    {
      arpg::rng generator(seed);

      for (const arpg::level_room& room : arpg::generate_level(recipe, generator).rooms)
      {
        // The station answers to its own rule, checked on its own above.
        if (room.role == arpg::room_role::station)
        {
          continue;
        }

        TTS_EXPECT(room.bounds.width >= recipe.room_min.x);
        TTS_EXPECT(room.bounds.height >= recipe.room_min.y);
        TTS_EXPECT(room.bounds.width <= recipe.room_max.x);
        TTS_EXPECT(room.bounds.height <= recipe.room_max.y);
      }
    }
  }
};

TTS_CASE("The two shapes do not lay out the same level")
{
  arpg::rng one(99);
  arpg::rng two(99);

  const arpg::level_layout square = arpg::generate_level(rigid, one);
  const arpg::level_layout grown = arpg::generate_level(organic, two);

  // The whole point of letting a biome choose: the same seed through two
  // shapes must not give the same place.
  const bool identical = square.rooms.size() == grown.rooms.size() &&
                         std::equal(square.rooms.begin(), square.rooms.end(), grown.rooms.begin(),
                                    [](const arpg::level_room& x, const arpg::level_room& y)
                                    { return x.bounds.x == y.bounds.x && x.bounds.y == y.bounds.y; });

  TTS_EXPECT_NOT(identical);
};

TTS_CASE("A recipe that asks for nothing lays out nothing")
{
  arpg::rng generator(3);

  TTS_EXPECT(arpg::generate_level(arpg::level_recipe{.rooms = 0}, generator).rooms.empty());
  TTS_EXPECT(arpg::generate_level(arpg::level_recipe{.rooms = -4}, generator).rooms.empty());

  // A minimum above the maximum has no size to draw from, and guessing which
  // of the two was meant would be worse than refusing.
  TTS_EXPECT(
      arpg::generate_level(arpg::level_recipe{.rooms = 6, .room_min = {500.0f, 500.0f}, .room_max = {100.0f, 100.0f}}, generator)
          .rooms.empty());
};

TTS_CASE("An empty level is connected, and asking about a room it lacks is safe")
{
  const arpg::level_layout nothing;

  TTS_EXPECT(arpg::fully_connected(nothing));
  TTS_EXPECT(arpg::neighbours_of(nothing, 17).empty());
};

TTS_CASE("Every link names rooms that exist, and none loops on itself")
{
  for (std::uint64_t seed = 1; seed <= 100; ++seed)
  {
    arpg::rng generator(seed);
    const arpg::level_layout level = arpg::generate_level(organic, generator);

    for (const arpg::level_link& link : level.links)
    {
      TTS_EXPECT(link.from < level.rooms.size());
      TTS_EXPECT(link.to < level.rooms.size());
      TTS_EXPECT(link.from != link.to);
    }
  }
};
