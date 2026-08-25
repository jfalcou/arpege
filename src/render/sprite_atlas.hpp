// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/vec2.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace arpg
{

/// One picture cut out of an atlas.
struct sprite_frame
{
  /// How animations and the data files name it.
  std::string name;

  /// Where it sits on the sheet, in pixels of the sheet.
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;

  /// Which point of the picture stands where the entity is, in pixels from
  /// its top left corner.
  ///
  /// Not the centre by default: seen from above, a body is drawn taller than
  /// the ground it occupies, so what marks its place is its feet. Getting this
  /// wrong is only noticed once everything is drawn, and fixing it afterwards
  /// moves every sprite in the game at once.
  vec2 origin{};
};

/// One picture held for a while.
struct animation_frame
{
  /// Names a frame of the atlas the animation belongs to.
  std::string frame;

  /// Seconds it is held. Per frame rather than one rate for the whole
  /// animation: holding the wind-up of a swing and flicking through the swing
  /// itself is most of what makes one read.
  float seconds = 0.1f;
};

struct sprite_animation
{
  std::string name;

  std::vector<animation_frame> frames;

  /// Whether it starts over. A walk loops; a death does not.
  bool loops = true;
};

/// A sheet, what is cut out of it, and what those cuts are played as.
struct sprite_atlas
{
  /// The image this describes, named beside the data file rather than by an
  /// absolute path: what is edited on one machine is loaded on another.
  std::string image;

  std::vector<sprite_frame> frames;
  std::vector<sprite_animation> animations;

  /// The frame answering to @p name, or nothing.
  const sprite_frame* find_frame(std::string_view name) const;

  /// The animation answering to @p name, or nothing.
  const sprite_animation* find_animation(std::string_view name) const;
};

/// How long an animation runs, which is what tells a played-once one when it
/// has finished.
float duration_of(const sprite_animation& clip);

/// Which frame of @p clip is showing after @p elapsed seconds.
///
/// A looping clip wraps; one that does not holds its last frame. The answer
/// indexes @p clip.frames, and is zero for a clip holding none, so the caller
/// checks for that before reaching in.
///
/// Pure, so what is drawn on a given step can be checked without drawing it.
std::size_t frame_at(const sprite_animation& clip, float elapsed);

} // namespace arpg
