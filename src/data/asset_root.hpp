// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <filesystem>
#include <string_view>

namespace arpg
{

/// Where the game reads its data from.
///
/// The build mirrors assets next to the executable, and that copy is
/// overwritten on the next build: tuning a figure there and then rebuilding
/// would lose the work without saying so. @p chosen, read from the
/// environment, points the game at the working copy instead, so the file being
/// edited live is the one under version control.
std::filesystem::path asset_root(std::string_view chosen, const std::filesystem::path& beside_executable);

/// Name of the environment variable @p chosen comes from.
inline constexpr const char* asset_root_variable = "ARPG_ASSETS";

} // namespace arpg
