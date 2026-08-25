// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <world/run_state.hpp>

#include <set>

TTS_CASE("A posting starts whole, owing nothing and holding nothing")
{
  const arpg::run_state run = arpg::begin_posting(1234, 3);

  TTS_EQUAL(run.seed, 1234U);
  TTS_EQUAL(run.health, 3);
  TTS_EQUAL(run.health_max, 3);
  TTS_EQUAL(run.banked, 0);
  TTS_EQUAL(run.carried, 0);
  TTS_EQUAL(run.depth, 0);
  TTS_EXPECT(arpg::employee_alive(run));
};

TTS_CASE("A level seed follows from the posting rather than being drawn")
{
  const arpg::run_state run = arpg::begin_posting(1234, 3);

  // A save keeping only the posting seed must still give back the same level,
  // and a seed shared between two players must give them the same one.
  TTS_EQUAL(arpg::level_seed(run, 0), arpg::level_seed(run, 0));
  TTS_EQUAL(arpg::current_level_seed(run), arpg::level_seed(run, 0));
};

TTS_CASE("Two depths of one posting are not two neighbouring levels")
{
  const arpg::run_state run = arpg::begin_posting(1234, 3);

  std::set<std::uint64_t> seen;

  for (int depth = 0; depth < 64; ++depth)
  {
    seen.insert(arpg::level_seed(run, depth));
  }

  // Without mixing, depth 0 and depth 1 would differ by one and lay out
  // levels that look like each other.
  TTS_EQUAL(seen.size(), 64U);
};

TTS_CASE("Two postings do not walk the same levels")
{
  const arpg::run_state one = arpg::begin_posting(1, 3);
  const arpg::run_state two = arpg::begin_posting(2, 3);

  TTS_EXPECT(arpg::level_seed(one, 0) != arpg::level_seed(two, 0));
  TTS_EXPECT(arpg::level_seed(one, 5) != arpg::level_seed(two, 5));
};

TTS_CASE("Finishing a level moves the posting on to another one")
{
  arpg::run_state run = arpg::begin_posting(7, 3);
  const std::uint64_t first = arpg::current_level_seed(run);

  arpg::finish_level(run);

  TTS_EQUAL(run.depth, 1);
  TTS_EXPECT(arpg::current_level_seed(run) != first);
};

TTS_CASE("What is sent back is safe, and what is carried is not")
{
  arpg::run_state run = arpg::begin_posting(7, 3);
  run.carried = 40;

  arpg::bank_loot(run);

  TTS_EQUAL(run.banked, 40);
  TTS_EQUAL(run.carried, 0);

  run.carried = 15;
  arpg::lose_employee(run);

  // The Bureau does not recognise what was never filed.
  TTS_EQUAL(run.carried, 0);
  TTS_EQUAL(run.banked, 40);
};

TTS_CASE("Losing an employee does not end the posting")
{
  arpg::run_state run = arpg::begin_posting(7, 3);
  arpg::finish_level(run);
  arpg::bank_loot(run);

  arpg::lose_employee(run);

  TTS_EXPECT_NOT(arpg::employee_alive(run));
  TTS_EQUAL(run.lost, 1);

  // The post outlives the person: the depth reached and what was filed both
  // stand, which is what makes sending someone else worth doing.
  TTS_EQUAL(run.depth, 1);
};

TTS_CASE("A new employee arrives whole and empty handed")
{
  arpg::run_state run = arpg::begin_posting(7, 3);
  run.carried = 90;
  arpg::lose_employee(run);

  arpg::assign_employee(run, 4);

  TTS_EXPECT(arpg::employee_alive(run));
  TTS_EQUAL(run.health, 4);
  TTS_EQUAL(run.health_max, 4);
  TTS_EQUAL(run.carried, 0);
};

TTS_CASE("A posting cannot be signed for nobody")
{
  // A profile that read zero health would otherwise put a corpse on the post.
  const arpg::run_state run = arpg::begin_posting(7, 0);

  TTS_EXPECT(arpg::employee_alive(run));
  TTS_EQUAL(run.health_max, 1);
};
