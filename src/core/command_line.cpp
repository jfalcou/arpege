// SPDX-License-Identifier: BSL-1.0

#include <core/command_line.hpp>

namespace arpg
{

namespace
{

constexpr std::string_view assets_option = "--assets";

} // namespace

std::string_view usage()
{
  return "usage: arpg [--assets DIR]\n"
         "\n"
         "  --assets DIR  read data files from DIR instead of the copy sitting\n"
         "                next to the executable. Overrides ARPG_ASSETS.\n"
         "  --help        print this and stop.\n";
}

launch_options parse_command_line(std::span<const std::string_view> arguments)
{
  launch_options out;

  for (std::size_t index = 0; index < arguments.size(); ++index)
  {
    const std::string_view argument = arguments[index];

    if (argument == "--help" || argument == "-h")
    {
      out.help = true;
      return out;
    }

    if (argument.starts_with(assets_option))
    {
      const std::string_view rest = argument.substr(assets_option.size());

      if (rest.empty())
      {
        // The value is the next argument, which there has to be one of.
        if (index + 1 >= arguments.size())
        {
          out.error = "--assets needs a directory";
          return out;
        }

        out.assets = std::string{arguments[++index]};
      }
      else if (rest.starts_with("="))
      {
        out.assets = std::string{rest.substr(1)};
      }
      else
      {
        out.error = "unknown option '" + std::string{argument} + "'";
        return out;
      }

      if (out.assets.empty())
      {
        out.error = "--assets needs a directory";
        return out;
      }

      continue;
    }

    out.error = "unknown option '" + std::string{argument} + "'";
    return out;
  }

  return out;
}

} // namespace arpg
