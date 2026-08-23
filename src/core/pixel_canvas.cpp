// SPDX-License-Identifier: BSL-1.0

#include "core/pixel_canvas.hpp"

#include "core/viewport.hpp"

namespace arpg
{

pixel_canvas::pixel_canvas(int width, int height)
  : m_width(width)
  , m_height(height)
{
  m_target = LoadRenderTexture(m_width, m_height);
  SetTextureFilter(m_target.texture, TEXTURE_FILTER_POINT);
}

pixel_canvas::~pixel_canvas()
{
  UnloadRenderTexture(m_target);
}

void pixel_canvas::begin_draw(Color clear) const
{
  BeginTextureMode(m_target);
  ClearBackground(clear);
}

void pixel_canvas::end_draw() const
{
  EndTextureMode();
}

int pixel_canvas::scale() const
{
  return integer_scale(m_width, m_height, GetScreenWidth(), GetScreenHeight());
}

Rectangle pixel_canvas::destination() const
{
  const viewport_rect rect = canvas_destination(m_width, m_height, GetScreenWidth(), GetScreenHeight());
  return Rectangle{rect.x, rect.y, rect.width, rect.height};
}

void pixel_canvas::present() const
{
  // Negative source height: OpenGL render textures are stored upside down.
  const Rectangle source{0.0f, 0.0f, static_cast<float>(m_target.texture.width),
                         -static_cast<float>(m_target.texture.height)};
  DrawTexturePro(m_target.texture, source, destination(), Vector2{0.0f, 0.0f}, 0.0f, WHITE);
}

Vector2 pixel_canvas::screen_to_canvas(Vector2 screen_position) const
{
  const viewport_point point = window_to_canvas(viewport_point{screen_position.x, screen_position.y}, m_width, m_height,
                                                GetScreenWidth(), GetScreenHeight());
  return Vector2{point.x, point.y};
}

} // namespace arpg
