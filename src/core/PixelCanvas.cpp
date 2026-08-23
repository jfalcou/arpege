#include "core/PixelCanvas.hpp"

#include <algorithm>

namespace arpg
{

PixelCanvas::PixelCanvas(int width, int height)
  : m_width(width)
  , m_height(height)
{
  m_target = LoadRenderTexture(m_width, m_height);
  SetTextureFilter(m_target.texture, TEXTURE_FILTER_POINT);
}

PixelCanvas::~PixelCanvas()
{
  UnloadRenderTexture(m_target);
}

void PixelCanvas::beginDraw(Color clear) const
{
  BeginTextureMode(m_target);
  ClearBackground(clear);
}

void PixelCanvas::endDraw() const
{
  EndTextureMode();
}

int PixelCanvas::scale() const
{
  const int byWidth = GetScreenWidth() / m_width;
  const int byHeight = GetScreenHeight() / m_height;
  return std::max(1, std::min(byWidth, byHeight));
}

Rectangle PixelCanvas::destination() const
{
  const float factor = static_cast<float>(scale());
  const float w = static_cast<float>(m_width) * factor;
  const float h = static_cast<float>(m_height) * factor;
  return Rectangle{(static_cast<float>(GetScreenWidth()) - w) * 0.5f,
                   (static_cast<float>(GetScreenHeight()) - h) * 0.5f, w, h};
}

void PixelCanvas::present() const
{
  // Negative source height: OpenGL render textures are stored upside down.
  const Rectangle source{0.0f, 0.0f, static_cast<float>(m_target.texture.width),
                         -static_cast<float>(m_target.texture.height)};
  DrawTexturePro(m_target.texture, source, destination(), Vector2{0.0f, 0.0f}, 0.0f, WHITE);
}

Vector2 PixelCanvas::screenToCanvas(Vector2 screenPosition) const
{
  const Rectangle dest = destination();
  const float factor = static_cast<float>(scale());
  return Vector2{(screenPosition.x - dest.x) / factor, (screenPosition.y - dest.y) / factor};
}

} // namespace arpg
