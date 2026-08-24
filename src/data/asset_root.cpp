// SPDX-License-Identifier: BSL-1.0

#include <data/asset_root.hpp>

namespace arpg
{

std::filesystem::path asset_root(std::string_view chosen, const std::filesystem::path& beside_executable)
{
  if (chosen.empty())
  {
    return beside_executable / "assets";
  }

  // Taken as given rather than appended to: the variable names the directory
  // itself, so it can point at a copy that is not called assets.
  return std::filesystem::path{chosen};
}

} // namespace arpg
