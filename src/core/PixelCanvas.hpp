#pragma once

#include <raylib.h>

namespace arpg
{

// Low resolution render target holding the world. Its content is upscaled to
// the window by an integer factor, with nearest neighbour filtering.
class PixelCanvas
{
public:
  PixelCanvas(int width, int height);
  ~PixelCanvas();

  PixelCanvas(const PixelCanvas&) = delete;
  PixelCanvas& operator=(const PixelCanvas&) = delete;

  void beginDraw(Color clear = BLACK) const;
  void endDraw() const;

  // Upscales the canvas to the window, centered, with letterboxing.
  void present() const;

  // Maps a window position, such as the mouse, to a canvas pixel.
  Vector2 screenToCanvas(Vector2 screenPosition) const;

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
