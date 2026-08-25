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
/// would lose the work without saying so. Either source points the game at the
/// working copy instead, so the file being edited live is the one under
/// version control.
///
/// The command line wins over the environment, which wins over the copy beside
/// the executable: what was typed for this run beats what a shell was left set
/// to.
std::filesystem::path asset_root(std::string_view from_command_line, std::string_view from_environment,
                                 const std::filesystem::path& beside_executable);

/// Name of the environment variable read into @p from_environment.
inline constexpr const char* asset_root_variable = "ARPG_ASSETS";

} // namespace arpg
