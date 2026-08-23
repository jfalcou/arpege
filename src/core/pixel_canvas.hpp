#pragma once

#include <raylib.h>

namespace arpg
{

// Low resolution render target holding the world. Its content is upscaled to
// the window by an integer factor, with nearest neighbour filtering.
class pixel_canvas
{
public:
  pixel_canvas(int width, int height);
  ~pixel_canvas();

  pixel_canvas(const pixel_canvas&) = delete;
  pixel_canvas& operator=(const pixel_canvas&) = delete;

  void begin_draw(Color clear = BLACK) const;
  void end_draw() const;

  // Upscales the canvas to the window, centered, with letterboxing.
  void present() const;

  // Maps a window position, such as the mouse, to a canvas pixel.
  Vector2 screen_to_canvas(Vector2 screen_position) const;

  int width() const { return m_width; }
  int height() const { return m_height; }
  const RenderTexture2D& target() const { return m_target; }

  // Window area covered by the upscaled canvas.
  Rectangle destination() const;

  // Current integer upscaling factor, at least 1.
  int scale() const;

private:
  int m_width;
  int m_height;
  RenderTexture2D m_target{};
};

} // namespace arpg
