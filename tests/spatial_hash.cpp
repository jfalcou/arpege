// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <ecs/spatial_hash.hpp>

#include <algorithm>

namespace
{

bool holds(const std::vector<entt::entity>& found, entt::entity looked_for)
{
  return std::find(found.begin(), found.end(), looked_for) != found.end();
}

constexpr auto first = static_cast<entt::entity>(1);
constexpr auto second = static_cast<entt::entity>(2);
constexpr auto third = static_cast<entt::entity>(3);

} // namespace

TTS_CASE("A query finds what sits at its centre")
{
  arpg::spatial_hash hash(32.0f);
  hash.insert(first, arpg::vec2{10.0f, 10.0f});

  std::vector<entt::entity> found;
  hash.query(arpg::vec2{10.0f, 10.0f}, 4.0f, found);

  TTS_EXPECT(holds(found, first));
};

TTS_CASE("A query ignores a distant cell entirely")
{
  arpg::spatial_hash hash(32.0f);
  hash.insert(first, arpg::vec2{10.0f, 10.0f});
  hash.insert(second, arpg::vec2{500.0f, 500.0f});

  std::vector<entt::entity> found;
  hash.query(arpg::vec2{10.0f, 10.0f}, 4.0f, found);

  TTS_EXPECT(holds(found, first));
  TTS_EXPECT_NOT(holds(found, second));
};

TTS_CASE("A query spanning a cell boundary finds both sides")
{
  arpg::spatial_hash hash(32.0f);

  // On either side of the boundary at x = 32.
  hash.insert(first, arpg::vec2{31.0f, 16.0f});
  hash.insert(second, arpg::vec2{33.0f, 16.0f});

  std::vector<entt::entity> found;
  hash.query(arpg::vec2{32.0f, 16.0f}, 4.0f, found);

  TTS_EXPECT(holds(found, first));
  TTS_EXPECT(holds(found, second));
};

TTS_CASE("Negative coordinates do not collide with positive ones")
{
  arpg::spatial_hash hash(32.0f);

  // Packing the cell coordinates badly makes these two share a key, which
  // would have one entity answer for the other.
  hash.insert(first, arpg::vec2{-40.0f, -40.0f});
  hash.insert(second, arpg::vec2{40.0f, 40.0f});

  std::vector<entt::entity> found;
  hash.query(arpg::vec2{40.0f, 40.0f}, 2.0f, found);

  TTS_EXPECT(holds(found, second));
  TTS_EXPECT_NOT(holds(found, first));
};

TTS_CASE("Results are candidates, not hits")
{
  arpg::spatial_hash hash(32.0f);

  // Same cell, but far enough apart that the circle misses it. The grid is
  // allowed to report it; the caller is the one testing distances.
  hash.insert(first, arpg::vec2{1.0f, 1.0f});

  std::vector<entt::entity> found;
  hash.query(arpg::vec2{30.0f, 30.0f}, 1.0f, found);

  TTS_EXPECT(holds(found, first));
};

TTS_CASE("Clearing empties the grid but keeps it usable")
{
  arpg::spatial_hash hash(32.0f);
  hash.insert(first, arpg::vec2{10.0f, 10.0f});
  hash.insert(second, arpg::vec2{200.0f, 200.0f});

  hash.clear();
  TTS_EQUAL(hash.occupied_cells(), 0U);

  std::vector<entt::entity> found;
  hash.query(arpg::vec2{10.0f, 10.0f}, 4.0f, found);
  TTS_EQUAL(found.size(), 0U);

  hash.insert(third, arpg::vec2{10.0f, 10.0f});
  hash.query(arpg::vec2{10.0f, 10.0f}, 4.0f, found);
  TTS_EXPECT(holds(found, third));
};

TTS_CASE("A query overwrites whatever the buffer held")
{
  arpg::spatial_hash hash(32.0f);
  hash.insert(first, arpg::vec2{10.0f, 10.0f});

  std::vector<entt::entity> found{second, third};
  hash.query(arpg::vec2{1000.0f, 1000.0f}, 4.0f, found);

  TTS_EQUAL(found.size(), 0U);
};

TTS_CASE("Several entities in one cell all come back")
{
  arpg::spatial_hash hash(32.0f);
  hash.insert(first, arpg::vec2{5.0f, 5.0f});
  hash.insert(second, arpg::vec2{6.0f, 6.0f});
  hash.insert(third, arpg::vec2{7.0f, 7.0f});

  std::vector<entt::entity> found;
  hash.query(arpg::vec2{6.0f, 6.0f}, 4.0f, found);

  TTS_EQUAL(found.size(), 3U);
  TTS_EQUAL(hash.occupied_cells(), 1U);
};

TTS_CASE("A degenerate cell size does not divide by zero")
{
  arpg::spatial_hash hash(0.0f);
  hash.insert(first, arpg::vec2{10.0f, 10.0f});

  std::vector<entt::entity> found;
  hash.query(arpg::vec2{10.0f, 10.0f}, 1.0f, found);

  TTS_EXPECT(holds(found, first));
};
