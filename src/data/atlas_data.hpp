// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <data/script_host.hpp>
#include <render/sprite_atlas.hpp>

#include <filesystem>
#include <string>
#include <string_view>

namespace arpg
{

/// An atlas that was read, or the reason it was not.
struct loaded_atlas
{
  sprite_atlas value;

  /// Empty when the file made sense. A refused file hands back nothing: half
  /// an atlas would draw the wrong picture rather than say anything.
  std::string error;

  bool valid() const { return error.empty(); }
};

loaded_atlas load_atlas(script_host& host, std::string_view source, std::string_view named);
loaded_atlas load_atlas_from(script_host& host, const std::filesystem::path& path);

/// Writes @p atlas back out as the Lua the reader takes.
///
/// The editor is the only thing that calls this, but it lives here beside the
/// reader on purpose: the two have to agree, and keeping them together lets a
/// test write an atlas, read it back, and hold the answer against what it
/// started from. That covers the whole format without opening a window.
std::string write_atlas(const sprite_atlas& atlas);

/// Whether @p name may be written out at all.
///
/// Names go into the file between quotes and nothing escapes them, so one
/// holding a quote or a newline would produce a file nobody can read back.
/// Refusing them beats writing an escaper for a mistake.
bool name_is_writable(std::string_view name);

} // namespace arpg
