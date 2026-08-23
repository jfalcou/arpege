// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <raylib.h>

namespace arpg
{

/// Low resolution render target holding the world.
///
/// The world is drawn here in logical pixels, then blown up to the window by a
/// whole number with nearest neighbour filtering, which is what keeps the
/// pixels square and sharp.
class pixel_canvas
{
public:
  /// Creates the render target, sized in logical pixels rather than window ones.
  pixel_canvas(int width, int height);
  ~pixel_canvas();

  pixel_canvas(const pixel_canvas&) = delete;
  pixel_canvas& operator=(const pixel_canvas&) = delete;

  /// Starts drawing into the canvas.
  void begin_draw(Color clear = BLACK) const;

  /// Ends drawing into the canvas.
  void end_draw() const;

  /// Blows the canvas up to the window, centered, with letterboxing.
  void present() const;

  /// Maps a window position, such as the mouse, to a canvas pixel.
  Vector2 screen_to_canvas(Vector2 screen_position) const;

  /// Canvas width in logical pixels.
  int width() const { return m_width; }

  /// Canvas height in logical pixels.
  int height() const { return m_height; }

  /// Underlying render texture, for a shader pass.
  const RenderTexture2D& target() const { return m_target; }

  /// Window area covered by the upscaled canvas.
  Rectangle destination() const;

  /// Current upscaling factor, at least 1.
  int scale() const;

private:
  int m_width;
  int m_height;
  RenderTexture2D m_target{};
};

} // namespace arpg
