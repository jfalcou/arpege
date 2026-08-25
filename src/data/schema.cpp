// SPDX-License-Identifier: BSL-1.0

#include <data/schema.hpp>

namespace arpg
{

std::string schema_error(const sol::table& described, std::string_view what, int understood)
{
  const int stated = described.get_or("version", 1);

  if (stated == understood)
  {
    return {};
  }

  return std::string{what} + " is written for version " + std::to_string(stated) + ", and this build reads version " +
         std::to_string(understood);
}

} // namespace arpg
