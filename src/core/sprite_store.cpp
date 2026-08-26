// SPDX-License-Identifier: BSL-1.0

#include <core/sprite_store.hpp>

#include <data/atlas_data.hpp>

namespace arpg
{

sprite_store::sprite_store(script_host& scripts, std::filesystem::path textures)
  : m_scripts(&scripts)
  , m_textures(std::move(textures))
{
  m_atlases = resource_cache<sprite_atlas>(
      [this](std::string_view name) -> std::optional<sprite_atlas>
      {
        const loaded_atlas read = load_atlas_from(*m_scripts, m_textures / (std::string{name} + ".lua"));

        if (!read.valid())
        {
          TraceLog(LOG_WARNING, "%s", read.error.c_str());
          return std::nullopt;
        }

        return read.value;
      });

  m_images = resource_cache<Texture2D>(
      [this](std::string_view name) -> std::optional<Texture2D>
      {
        const Texture2D loaded = LoadTexture((m_textures / name).string().c_str());

        if (loaded.id == 0)
        {
          return std::nullopt;
        }

        // Nearest, always. A pixel smoothed into its neighbour is the one thing
        // this whole pipeline exists to avoid.
        SetTextureFilter(loaded, TEXTURE_FILTER_POINT);

        return loaded;
      },
      [](Texture2D& held) { UnloadTexture(held); });
}

sheet sprite_store::get(std::string_view name)
{
  const sprite_atlas* atlas = m_atlases.get(name);

  if (atlas == nullptr)
  {
    return sheet{};
  }

  const Texture2D* image = m_images.get(atlas->image);

  if (image == nullptr)
  {
    return sheet{};
  }

  return sheet{atlas, image};
}

void sprite_store::clear()
{
  m_atlases.clear();
  m_images.clear();
}

} // namespace arpg
