// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/vec2.hpp>

namespace arpg
{

/// What a dash is worth, in figures a data file states.
struct dash_profile
{
  /// How fast it carries, well above walking or it would not read as an
  /// escape.
  float speed = 260.0f;

  /// How long it lasts. Short: a dash is a commitment, and a long one would
  /// let the player cross the room rather than slip through a gap.
  float duration = 0.14f;

  /// From the start of one dash to the earliest start of the next, so the
  /// figure means the same whatever the duration is.
  float cooldown = 0.55f;

  /// Seconds of invulnerability the dash grants. This is what makes a wall of
  /// bullets a thing to read rather than a thing to avoid: the answer is to
  /// pass through it at the right moment.
  float mercy = 0.14f;
};

/// Where a dash currently stands.
struct dash_state
{
  /// Seconds of dash left. Anything above zero means the heading is locked.
  float remaining = 0.0f;

  /// Seconds before another one may start.
  float cooldown = 0.0f;

  /// The heading it left along, held for its whole duration.
  vec2 heading{};

  /// Last direction the player asked for, which is what a dash asked for while
  /// standing still goes along.
  vec2 facing{};
};

/// True while the dash is carrying the player, so the caller knows to ignore
/// the steering.
bool dashing(const dash_state& dash);

/// The velocity a dash imposes. Zero when none is running.
vec2 dash_velocity(const dash_state& dash, const dash_profile& profile);

/// Advances @p dash by @p dt.
///
/// @p wants is whether the action was asked for on this step, and @p steering
/// the direction the player is holding, which may be zero.
///
/// Returns true on the step a dash starts, which is when the caller grants the
/// invulnerability: doing it here would mean this knowing about the world.
bool advance_dash(dash_state& dash, const dash_profile& profile, bool wants, vec2 steering, float dt);

} // namespace arpg
