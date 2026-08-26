// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/resource_cache.hpp>
#include <data/script_host.hpp>
#include <render/sprite_atlas.hpp>

#include <raylib.h>

#include <filesystem>
#include <string_view>

namespace arpg
{

/// A sheet and what is cut out of it, held together.
///
/// Either half being missing makes the pair useless, so nothing hands back one
/// without the other.
struct sheet
{
  const sprite_atlas* atlas = nullptr;
  const Texture2D* texture = nullptr;

  bool usable() const { return atlas != nullptr && texture != nullptr; }
};

/// Where the game gets its pictures.
///
/// The one place that touches raylib for them, and the one place that knows an
/// atlas is a file. What it holds is loaded once and handed out by name; the
/// caching itself is resource_cache, which knows none of this.
class sprite_store
{
public:
  /// @p textures is where both the atlas files and the images they name live.
  sprite_store(script_host& scripts, std::filesystem::path textures);

  /// The sheet called @p name, from @p name.lua and the image it names.
  ///
  /// Both halves come back or neither does, and a name that could not be had
  /// is not looked for again.
  sheet get(std::string_view name);

  /// Lets go of everything, and looks again for what was missing.
  void clear();

private:
  script_host* m_scripts = nullptr;
  std::filesystem::path m_textures;

  resource_cache<sprite_atlas> m_atlases;
  resource_cache<Texture2D> m_images;
};

} // namespace arpg
