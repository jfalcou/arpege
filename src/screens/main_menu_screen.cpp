// SPDX-License-Identifier: BSL-1.0

#include "screens/main_menu_screen.hpp"

#include "core/pixel_canvas.hpp"

namespace arpg
{

void main_menu_screen::on_enter()
{
  const pixel_canvas& canvas = *ctx().canvas;
  m_current = Vector2{static_cast<float>(canvas.width()) * 0.5f, static_cast<float>(canvas.height()) * 0.62f};
  m_previous = m_current;

  // Non integer speeds, so that a missing interpolation shows up as stutter.
  m_velocity = Vector2{74.0f, 41.0f};
}

void main_menu_screen::update(float dt)
{
  if (IsKeyPressed(KEY_ESCAPE))
  {
    ctx().request_quit();
    return;
  }

  const pixel_canvas& canvas = *ctx().canvas;
  const float min_x = m_radius;
  const float max_x = static_cast<float>(canvas.width()) - m_radius;
  const float min_y = static_cast<float>(canvas.height()) * 0.45f;
  const float max_y = static_cast<float>(canvas.height()) - m_radius;

  m_previous = m_current;
  m_current.x += m_velocity.x * dt;
  m_current.y += m_velocity.y * dt;

  if (m_current.x < min_x || m_current.x > max_x)
  {
    m_velocity.x = -m_velocity.x;
    m_current.x = (m_current.x < min_x) ? min_x : max_x;
  }

  if (m_current.y < min_y || m_current.y > max_y)
  {
    m_velocity.y = -m_velocity.y;
    m_current.y = (m_current.y < min_y) ? min_y : max_y;
  }
}

void main_menu_screen::render(float alpha)
{
  const pixel_canvas& canvas = *ctx().canvas;
  ClearBackground(Color{18, 14, 26, 255});

  const char* title = "ARPG";
  const int title_size = 20;
  DrawText(title, (canvas.width() - MeasureText(title, title_size)) / 2, 34, title_size, Color{226, 205, 154, 255});

  const char* hint = "F1 debug   -   ESC quit";
  const int hint_size = 10;
  DrawText(hint, (canvas.width() - MeasureText(hint, hint_size)) / 2, 62, hint_size, Color{120, 110, 130, 255});

  const Vector2 drawn{m_previous.x + (m_current.x - m_previous.x) * alpha,
                      m_previous.y + (m_current.y - m_previous.y) * alpha};
  DrawCircleV(drawn, m_radius, Color{198, 88, 78, 255});
}

} // namespace arpg
