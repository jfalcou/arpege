// SPDX-License-Identifier: BSL-1.0

#include <data/asset_root.hpp>

namespace arpg
{

std::filesystem::path asset_root(std::string_view from_command_line, std::string_view from_environment,
                                 const std::filesystem::path& beside_executable)
{
  // Taken as given rather than appended to: either source names the directory
  // itself, so it can point at a copy that is not called assets.
  if (!from_command_line.empty())
  {
    return std::filesystem::path{from_command_line};
  }

  if (!from_environment.empty())
  {
    return std::filesystem::path{from_environment};
  }

  return beside_executable / "assets";
}

} // namespace arpg
