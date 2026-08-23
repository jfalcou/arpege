#include "core/PixelCanvas.hpp"

#include "core/Viewport.hpp"

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
  return integerScale(m_width, m_height, GetScreenWidth(), GetScreenHeight());
}

Rectangle PixelCanvas::destination() const
{
  const ViewportRect rect = canvasDestination(m_width, m_height, GetScreenWidth(), GetScreenHeight());
  return Rectangle{rect.x, rect.y, rect.width, rect.height};
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
  const ViewportPoint point = windowToCanvas(ViewportPoint{screenPosition.x, screenPosition.y}, m_width, m_height,
                                             GetScreenWidth(), GetScreenHeight());
  return Vector2{point.x, point.y};
}

} // namespace arpg
