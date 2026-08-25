// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <sol/sol.hpp>

#include <string>
#include <string_view>

namespace arpg
{

/// What the loaders in this build understand, one number per kind of file.
///
/// Bumped when a change to a format would make a file written for the previous
/// one mean something else. A field merely added does not need it: a loader
/// that refuses what it does not recognise would refuse too much.
inline constexpr int enemies_schema = 1;
inline constexpr int player_schema = 1;
inline constexpr int biome_schema = 1;

/// Empty when @p described is written for a version this build reads.
///
/// A data file normally travels with the binary, so this is worth nothing
/// until someone drops a file of their own beside it. Then it is the
/// difference between "this biome has no fauna" and "this file is written for
/// a version this build no longer reads".
///
/// A file that states no version is taken for the first one, which is what
/// makes the field possible to add after the fact.
std::string schema_error(const sol::table& described, std::string_view what, int understood);

} // namespace arpg
