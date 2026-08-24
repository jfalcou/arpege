// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <sol/sol.hpp>

#include <filesystem>
#include <string>
#include <string_view>

namespace arpg
{

/// A script that ran, or the reason it did not.
///
/// The table borrows the host it came from and must not outlive it, which is
/// why loaders turn one into plain structs before returning.
struct script_result
{
  sol::table value;
  std::string error;

  bool valid() const { return error.empty(); }
};

/// The single Lua state the data files run in.
///
/// One host rather than one per loader: the sandbox below is a policy, and a
/// policy repeated at every call site is a policy that will eventually differ
/// from itself. Later loaders, and the hot reload, all come through here.
class script_host
{
public:
  script_host();

  /// Runs @p source and hands back the table it returns. @p name is what
  /// errors are attributed to, so a message points at a file rather than at
  /// an anonymous chunk.
  script_result run(std::string_view source, std::string_view name);

  /// Same, reading @p path. A file that cannot be opened is an error like any
  /// other rather than an exception.
  script_result run_file(const std::filesystem::path& path);

private:
  sol::state m_lua;
};

} // namespace arpg
