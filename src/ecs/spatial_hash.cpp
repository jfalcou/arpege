// SPDX-License-Identifier: BSL-1.0

#include <ecs/spatial_hash.hpp>

#include <algorithm>
#include <cmath>

namespace arpg
{

spatial_hash::spatial_hash(float cell_size)
  : m_cell_size(cell_size > 0.0f ? cell_size : 1.0f)
{
}

std::int64_t spatial_hash::key_of(std::int32_t x, std::int32_t y)
{
  // Both halves are kept whole: shifting a signed value into the upper bits
  // would collide for negative coordinates, and the play area has plenty.
  const std::uint64_t low = static_cast<std::uint32_t>(x);
  const std::uint64_t high = static_cast<std::uint32_t>(y);
  return static_cast<std::int64_t>((high << 32) | low);
}

std::int32_t spatial_hash::cell_of(float value) const
{
  return static_cast<std::int32_t>(std::floor(value / m_cell_size));
}

void spatial_hash::clear()
{
  // The cells are emptied rather than dropped, so the next step reuses the
  // buckets already allocated instead of asking for new ones.
  for (auto& entry : m_cells)
  {
    entry.second.clear();
  }
}

void spatial_hash::insert(entt::entity value, vec2 position)
{
  m_cells[key_of(cell_of(position.x), cell_of(position.y))].push_back(value);
}

void spatial_hash::query(vec2 centre, float radius, std::vector<entt::entity>& out) const
{
  out.clear();

  const std::int32_t min_x = cell_of(centre.x - radius);
  const std::int32_t max_x = cell_of(centre.x + radius);
  const std::int32_t min_y = cell_of(centre.y - radius);
  const std::int32_t max_y = cell_of(centre.y + radius);

  for (std::int32_t y = min_y; y <= max_y; ++y)
  {
    for (std::int32_t x = min_x; x <= max_x; ++x)
    {
      const auto found = m_cells.find(key_of(x, y));

      if (found != m_cells.end())
      {
        out.insert(out.end(), found->second.begin(), found->second.end());
      }
    }
  }
}

std::size_t spatial_hash::occupied_cells() const
{
  return static_cast<std::size_t>(
      std::count_if(m_cells.begin(), m_cells.end(), [](const auto& entry) { return !entry.second.empty(); }));
}

} // namespace arpg
