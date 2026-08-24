// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/vec2.hpp>

#include <entt/entity/entity.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace arpg
{

/// Uniform grid over the play area, rebuilt every step.
///
/// Testing every bullet against every target is quadratic and collapses well
/// before the thousands a bullet hell throws around. Bucketing by cell turns a
/// query into a look at the handful of cells a radius actually covers.
///
/// A uniform grid rather than a tree: rebuilding it is a linear pass with no
/// allocation once warm, and bullets move too much for a tree to pay off.
class spatial_hash
{
public:
  /// @param cell_size side of a cell, in canvas pixels. It must be at least as
  ///        large as the biggest collider in play, since a query only widens by
  ///        one cell and a larger target could otherwise sit in a cell that is
  ///        never visited. Smaller multiplies the cells to walk, larger puts too
  ///        much in each one.
  explicit spatial_hash(float cell_size);

  /// Empties every cell, keeping the memory for the next step.
  void clear();

  /// Files @p value under the cell containing @p position.
  void insert(entt::entity value, vec2 position);

  /// Appends every entity filed in a cell the circle touches to @p out.
  ///
  /// Results are candidates, not hits: a cell overlapping the circle may hold
  /// entities outside it, so the caller still tests distances. @p out is
  /// cleared first, and reusing the same vector across queries avoids
  /// reallocating on every one.
  void query(vec2 centre, float radius, std::vector<entt::entity>& out) const;

  /// Cell side, as given at construction.
  float cell_size() const { return m_cell_size; }

  /// Number of non-empty cells, for tests and the debug overlay.
  std::size_t occupied_cells() const;

private:
  /// Cells are keyed by their coordinates packed into one integer, which keeps
  /// the map simple and the hashing cheap.
  static std::int64_t key_of(std::int32_t x, std::int32_t y);

  std::int32_t cell_of(float value) const;

  float m_cell_size;
  std::unordered_map<std::int64_t, std::vector<entt::entity>> m_cells;
};

} // namespace arpg
