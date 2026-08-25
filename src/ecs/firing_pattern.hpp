// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/vec2.hpp>

#include <cstdint>
#include <vector>

namespace arpg
{

/// Where the volley points from.
enum class aim_mode : std::uint8_t
{
  /// At the player, so the volley follows them.
  aimed,

  /// At a heading of its own, which only the spin moves. A wall of bullets
  /// that ignores where the player stands is what makes a room a place to
  /// read rather than a duel.
  fixed
};

/// A volley described by three figures rather than by a name.
///
/// Aimed shot, fan, circle and spiral are not four cases: they are what comes
/// out of a count, an arc and a rotation per volley. A fan is an arc over
/// several bullets, a circle is an arc of a full turn, a spiral is a single
/// bullet with a spin. Naming them separately would have meant four code paths
/// that cannot be combined.
struct firing_pattern
{
  aim_mode aim = aim_mode::aimed;

  /// How many bullets leave at once.
  int bullets = 1;

  /// Degrees the volley spans. Zero stacks every bullet on one heading, and a
  /// full turn spreads them around.
  float arc = 0.0f;

  /// Degrees added to the heading at every volley.
  float spin = 0.0f;

  /// Seconds between volleys.
  float interval = 1.2f;

  float speed = 70.0f;
  float radius = 2.0f;
  int damage = 1;

  /// Seconds before a bullet that hit nothing gives up.
  float life = 4.0f;
};

/// Fills @p out with the headings of volley number @p volley.
///
/// @p towards is the direction of the player, used only by an aimed pattern
/// and normalised by the caller. Clears @p out first, and leaves it empty for
/// a pattern that fires nothing.
///
/// Pure and deterministic: the same volley number gives the same headings, so
/// a replay reproduces a wall of bullets exactly.
void volley_headings(const firing_pattern& pattern, vec2 towards, int volley, std::vector<vec2>& out);

} // namespace arpg
