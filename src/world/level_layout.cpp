// SPDX-License-Identifier: BSL-1.0

#include <world/level_layout.hpp>

#include <algorithm>
#include <cmath>
#include <queue>

namespace arpg
{

namespace
{

/// How many times a room is offered a place before the generator gives up on
/// it. Without a bound, a recipe asking for more rooms than fit would spin.
constexpr int placement_attempts = 24;

bool overlaps(const viewport_rect& a, const viewport_rect& b, float margin)
{
  return a.x - margin < b.x + b.width && b.x - margin < a.x + a.width && a.y - margin < b.y + b.height &&
         b.y - margin < a.y + a.height;
}

bool fits(const std::vector<level_room>& rooms, const viewport_rect& candidate, float margin)
{
  return std::none_of(rooms.begin(), rooms.end(),
                      [&](const level_room& room) { return overlaps(room.bounds, candidate, margin); });
}

/// Picks a number in [low, high), which is what every dimension here needs.
float between(rng& generator, float low, float high)
{
  return low + generator.unit() * (high - low);
}

// --- rigid ------------------------------------------------------------------

/// Splits @p box until it is too small to halve, and keeps the leaves.
///
/// Recursive rather than iterative because the recursion is the algorithm: a
/// leaf is a box nobody could split, and that reads directly.
void split(const viewport_rect& box, const level_recipe& recipe, int depth, rng& generator,
           std::vector<viewport_rect>& leaves)
{
  const float least = recipe.room_min + recipe.spacing;
  const bool can_split_x = box.width >= 2.0f * least;
  const bool can_split_y = box.height >= 2.0f * least;

  if (depth <= 0 || (!can_split_x && !can_split_y))
  {
    leaves.push_back(box);
    return;
  }

  // The longer side is cut, so rooms stay closer to square than to corridors.
  const bool cut_x = can_split_x && (!can_split_y || box.width >= box.height);
  const float span = cut_x ? box.width : box.height;

  // Never at the middle, or every box halved exactly would give a grid, and a
  // grid does not read as architecture. Kept a whole plot away from either end
  // all the same: cutting at a fraction of the span alone would leave a piece
  // too small to hold the room it is supposed to receive.
  const float at = between(generator, least, span - least);

  if (cut_x)
  {
    split(viewport_rect{box.x, box.y, at, box.height}, recipe, depth - 1, generator, leaves);
    split(viewport_rect{box.x + at, box.y, box.width - at, box.height}, recipe, depth - 1, generator, leaves);
  }
  else
  {
    split(viewport_rect{box.x, box.y, box.width, at}, recipe, depth - 1, generator, leaves);
    split(viewport_rect{box.x, box.y + at, box.width, box.height - at}, recipe, depth - 1, generator, leaves);
  }
}

level_layout lay_out_rigid(const level_recipe& recipe, rng& generator)
{
  level_layout out;

  // Depth is what bounds the room count, since every split doubles the leaves.
  int depth = 0;
  while ((1 << depth) < recipe.rooms)
  {
    ++depth;
  }

  const float side = std::ceil(std::sqrt(static_cast<float>(1 << depth))) * (recipe.room_max + recipe.spacing);

  std::vector<viewport_rect> leaves;
  split(viewport_rect{0.0f, 0.0f, side, side}, recipe, depth, generator, leaves);

  for (const viewport_rect& leaf : leaves)
  {
    // The leaf is the plot, not the room. Inset for a corridor, then capped at
    // what the recipe allows and centred on what is left: a leaf larger than a
    // room is simply a room with ground around it, where a room stretched to
    // its plot would ignore the size it was asked for.
    const float plot_width = std::max(1.0f, leaf.width - recipe.spacing);
    const float plot_height = std::max(1.0f, leaf.height - recipe.spacing);

    const float width = std::min(plot_width, recipe.room_max);
    const float height = std::min(plot_height, recipe.room_max);

    out.rooms.push_back(level_room{
        viewport_rect{leaf.x + (leaf.width - width) * 0.5f, leaf.y + (leaf.height - height) * 0.5f, width, height},
        room_role::fight});
  }

  // Linked along the order the split produced, which walks the tree: two rooms
  // next to each other in that order are siblings or cousins, so they sit side
  // by side on the map.
  for (std::size_t index = 1; index < out.rooms.size(); ++index)
  {
    out.links.push_back(level_link{index - 1, index});
  }

  return out;
}

// --- organic ----------------------------------------------------------------

level_layout lay_out_organic(const level_recipe& recipe, rng& generator)
{
  level_layout out;

  out.rooms.push_back(level_room{viewport_rect{0.0f, 0.0f, between(generator, recipe.room_min, recipe.room_max),
                                               between(generator, recipe.room_min, recipe.room_max)},
                                 room_role::fight});

  while (static_cast<int>(out.rooms.size()) < recipe.rooms)
  {
    bool placed = false;

    for (int attempt = 0; attempt < placement_attempts && !placed; ++attempt)
    {
      const std::size_t against = generator.below(static_cast<std::uint32_t>(out.rooms.size()));
      const viewport_rect& host = out.rooms[against].bounds;

      const float width = between(generator, recipe.room_min, recipe.room_max);
      const float height = between(generator, recipe.room_min, recipe.room_max);

      // Slid along the shared wall rather than centred on it, or every room
      // would line up on the axis of the one it hangs from.
      const float slide = between(generator, -0.4f, 0.4f);

      viewport_rect candidate{0.0f, 0.0f, width, height};

      switch (generator.below(4))
      {
      case 0:
        candidate.x = host.x + slide * host.width;
        candidate.y = host.y - height - recipe.spacing;
        break;
      case 1:
        candidate.x = host.x + host.width + recipe.spacing;
        candidate.y = host.y + slide * host.height;
        break;
      case 2:
        candidate.x = host.x + slide * host.width;
        candidate.y = host.y + host.height + recipe.spacing;
        break;
      default:
        candidate.x = host.x - width - recipe.spacing;
        candidate.y = host.y + slide * host.height;
        break;
      }

      if (!fits(out.rooms, candidate, recipe.spacing * 0.5f))
      {
        continue;
      }

      out.rooms.push_back(level_room{candidate, room_role::fight});
      out.links.push_back(level_link{against, out.rooms.size() - 1});
      placed = true;
    }

    // Nowhere left to hang a room: the recipe asked for more than the shape
    // can hold, and stopping beats spinning.
    if (!placed)
    {
      break;
    }
  }

  return out;
}

// --- roles ------------------------------------------------------------------

/// Steps from @p from to every room, or -1 for one that cannot be reached.
std::vector<int> distances_from(const level_layout& layout, std::size_t from)
{
  std::vector<int> steps(layout.rooms.size(), -1);

  if (from >= layout.rooms.size())
  {
    return steps;
  }

  std::queue<std::size_t> pending;
  steps[from] = 0;
  pending.push(from);

  while (!pending.empty())
  {
    const std::size_t here = pending.front();
    pending.pop();

    for (const std::size_t next : neighbours_of(layout, here))
    {
      if (steps[next] < 0)
      {
        steps[next] = steps[here] + 1;
        pending.push(next);
      }
    }
  }

  return steps;
}

void assign_roles(level_layout& layout, rng& generator)
{
  if (layout.rooms.empty())
  {
    return;
  }

  layout.start = generator.below(static_cast<std::uint32_t>(layout.rooms.size()));

  // The boss sits as far from the entrance as the level allows, so reaching it
  // means having crossed the level rather than having turned a corner.
  const std::vector<int> steps = distances_from(layout, layout.start);
  layout.boss = layout.start;

  for (std::size_t index = 0; index < layout.rooms.size(); ++index)
  {
    if (steps[index] > steps[layout.boss])
    {
      layout.boss = index;
    }
  }

  for (level_room& room : layout.rooms)
  {
    room.role = room_role::fight;
  }

  layout.rooms[layout.start].role = room_role::start;
  layout.rooms[layout.boss].role = room_role::boss;

  // The station goes on a dead end wherever there is one: reaching it has to
  // cost a detour, or sending loot back would never be a decision.
  std::vector<std::size_t> dead_ends;

  for (std::size_t index = 0; index < layout.rooms.size(); ++index)
  {
    if (index != layout.start && index != layout.boss && neighbours_of(layout, index).size() == 1)
    {
      dead_ends.push_back(index);
    }
  }

  // A shape without any dead end still gets a station: one that only exists on
  // some layouts would make the loop it drives come and go.
  if (dead_ends.empty())
  {
    for (std::size_t index = 0; index < layout.rooms.size(); ++index)
    {
      if (index != layout.start && index != layout.boss)
      {
        dead_ends.push_back(index);
      }
    }
  }

  if (!dead_ends.empty())
  {
    layout.rooms[dead_ends[generator.below(static_cast<std::uint32_t>(dead_ends.size()))]].role = room_role::station;
  }
}

} // namespace

std::vector<std::size_t> neighbours_of(const level_layout& layout, std::size_t room)
{
  std::vector<std::size_t> found;

  for (const level_link& link : layout.links)
  {
    if (link.from == room)
    {
      found.push_back(link.to);
    }
    else if (link.to == room)
    {
      found.push_back(link.from);
    }
  }

  return found;
}

bool fully_connected(const level_layout& layout)
{
  if (layout.rooms.empty())
  {
    return true;
  }

  const std::vector<int> steps = distances_from(layout, layout.start);

  return std::none_of(steps.begin(), steps.end(), [](int reached) { return reached < 0; });
}

level_layout generate_level(const level_recipe& recipe, rng& generator)
{
  if (recipe.rooms <= 0 || recipe.room_min <= 0.0f || recipe.room_max < recipe.room_min)
  {
    return level_layout{};
  }

  level_layout out =
      (recipe.shape == level_shape::organic) ? lay_out_organic(recipe, generator) : lay_out_rigid(recipe, generator);

  assign_roles(out, generator);

  return out;
}

} // namespace arpg
