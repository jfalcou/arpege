// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstddef>

namespace arpg
{

/// Raised once, when the last enemy of a room falls.
///
/// Screens never call each other, so this is how the room tells whatever runs
/// the strate above it that it is done.
struct room_cleared
{
  std::size_t enemies_defeated = 0;
};

} // namespace arpg
