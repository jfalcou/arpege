// SPDX-License-Identifier: BSL-1.0

#include <render/sprite_atlas.hpp>

#include <algorithm>
#include <cmath>

namespace arpg
{

const sprite_frame* sprite_atlas::find_frame(std::string_view name) const
{
  const auto found =
      std::find_if(frames.begin(), frames.end(), [name](const sprite_frame& each) { return each.name == name; });

  return (found == frames.end()) ? nullptr : &(*found);
}

const sprite_animation* sprite_atlas::find_animation(std::string_view name) const
{
  const auto found = std::find_if(animations.begin(), animations.end(),
                                  [name](const sprite_animation& each) { return each.name == name; });

  return (found == animations.end()) ? nullptr : &(*found);
}

std::vector<sprite_frame> slice_grid(int image_width, int image_height, const grid_slice& how)
{
  std::vector<sprite_frame> cut;

  if (how.cell_width <= 0 || how.cell_height <= 0 || how.margin < 0 || how.spacing < 0)
  {
    return cut;
  }

  const int step_x = how.cell_width + how.spacing;
  const int step_y = how.cell_height + how.spacing;

  int number = 0;

  for (int y = how.margin; y + how.cell_height <= image_height; y += step_y)
  {
    for (int x = how.margin; x + how.cell_width <= image_width; x += step_x)
    {
      sprite_frame frame;
      frame.name = how.prefix + "_" + std::to_string(number);
      frame.x = x;
      frame.y = y;
      frame.width = how.cell_width;
      frame.height = how.cell_height;
      frame.origin =
          vec2{how.origin.x * static_cast<float>(how.cell_width), how.origin.y * static_cast<float>(how.cell_height)};

      cut.push_back(std::move(frame));
      ++number;
    }
  }

  return cut;
}

float duration_of(const sprite_animation& clip)
{
  float total = 0.0f;

  for (const animation_frame& each : clip.frames)
  {
    total += std::max(0.0f, each.seconds);
  }

  return total;
}

std::size_t frame_at(const sprite_animation& clip, float elapsed)
{
  if (clip.frames.empty())
  {
    return 0;
  }

  const float total = duration_of(clip);

  // Frames of no length at all: nothing can be shown for a share of nothing,
  // so the first one stands rather than the walk dividing by zero.
  if (total <= 0.0f)
  {
    return 0;
  }

  float when = std::max(0.0f, elapsed);

  if (clip.loops)
  {
    when = std::fmod(when, total);
  }
  else if (when >= total)
  {
    // Held rather than wrapped: a death that started over would be a strange
    // thing to watch.
    return clip.frames.size() - 1;
  }

  for (std::size_t index = 0; index < clip.frames.size(); ++index)
  {
    when -= std::max(0.0f, clip.frames[index].seconds);

    if (when < 0.0f)
    {
      return index;
    }
  }

  // Only reachable on the rounding of the last frame's edge.
  return clip.frames.size() - 1;
}

} // namespace arpg
