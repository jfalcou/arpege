// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <span>
#include <string>
#include <string_view>

namespace arpg
{

/// What the command line asked for.
struct launch_options
{
  /// Where to read data from, empty when the option was not given.
  std::string assets;

  /// Set when the usage was asked for, so the caller prints it and stops.
  bool help = false;

  /// Empty when the line made sense. An unknown option is refused rather than
  /// ignored: a misspelt one would otherwise look like it worked.
  std::string error;

  bool valid() const { return error.empty(); }
};

/// Reads @p arguments, which excludes the program name.
///
/// An option takes its value either glued with an equals sign or as the next
/// argument, since both are what people type.
launch_options parse_command_line(std::span<const std::string_view> arguments);

/// What to print when the line is wrong, or when it asks.
std::string_view usage();

} // namespace arpg
