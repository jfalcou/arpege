#include "screens/MainMenuScreen.hpp"

#include "core/PixelCanvas.hpp"

namespace arpg
{

void MainMenuScreen::onEnter()
{
  const PixelCanvas& canvas = *ctx().canvas;
  m_current = Vector2{static_cast<float>(canvas.width()) * 0.5f, static_cast<float>(canvas.height()) * 0.62f};
  m_previous = m_current;

  // Non integer speeds, so that a missing interpolation shows up as stutter.
  m_velocity = Vector2{74.0f, 41.0f};
}

void MainMenuScreen::update(float dt)
{
  if (IsKeyPressed(KEY_ESCAPE))
  {
    ctx().requestQuit();
    return;
  }

  const PixelCanvas& canvas = *ctx().canvas;
  const float minX = m_radius;
  const float maxX = static_cast<float>(canvas.width()) - m_radius;
  const float minY = static_cast<float>(canvas.height()) * 0.45f;
  const float maxY = static_cast<float>(canvas.height()) - m_radius;

  m_previous = m_current;
  m_current.x += m_velocity.x * dt;
  m_current.y += m_velocity.y * dt;

  if (m_current.x < minX || m_current.x > maxX)
  {
    m_velocity.x = -m_velocity.x;
    m_current.x = (m_current.x < minX) ? minX : maxX;
  }

  if (m_current.y < minY || m_current.y > maxY)
  {
    m_velocity.y = -m_velocity.y;
    m_current.y = (m_current.y < minY) ? minY : maxY;
  }
}

void MainMenuScreen::render(float alpha)
{
  const PixelCanvas& canvas = *ctx().canvas;
  ClearBackground(Color{18, 14, 26, 255});

  const char* title = "ARPG";
  const int titleSize = 20;
  DrawText(title, (canvas.width() - MeasureText(title, titleSize)) / 2, 34, titleSize, Color{226, 205, 154, 255});

  const char* hint = "F1 debug   -   ESC quit";
  const int hintSize = 10;
  DrawText(hint, (canvas.width() - MeasureText(hint, hintSize)) / 2, 62, hintSize, Color{120, 110, 130, 255});

  const Vector2 drawn{m_previous.x + (m_current.x - m_previous.x) * alpha,
                      m_previous.y + (m_current.y - m_previous.y) * alpha};
  DrawCircleV(drawn, m_radius, Color{198, 88, 78, 255});
}

} // namespace arpg
