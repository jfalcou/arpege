#include "core/Viewport.hpp"

#include <algorithm>

namespace arpg
{

int integerScale(int canvasWidth, int canvasHeight, int windowWidth, int windowHeight)
{
  if (canvasWidth <= 0 || canvasHeight <= 0)
  {
    return 1;
  }

  const int byWidth = windowWidth / canvasWidth;
  const int byHeight = windowHeight / canvasHeight;
  return std::max(1, std::min(byWidth, byHeight));
}

ViewportRect canvasDestination(int canvasWidth, int canvasHeight, int windowWidth, int windowHeight)
{
  const float factor = static_cast<float>(integerScale(canvasWidth, canvasHeight, windowWidth, windowHeight));
  const float width = static_cast<float>(canvasWidth) * factor;
  const float height = static_cast<float>(canvasHeight) * factor;

  return ViewportRect{(static_cast<float>(windowWidth) - width) * 0.5f,
                      (static_cast<float>(windowHeight) - height) * 0.5f, width, height};
}

ViewportPoint windowToCanvas(ViewportPoint windowPosition, int canvasWidth, int canvasHeight, int windowWidth,
                             int windowHeight)
{
  const ViewportRect dest = canvasDestination(canvasWidth, canvasHeight, windowWidth, windowHeight);
  const float factor = static_cast<float>(integerScale(canvasWidth, canvasHeight, windowWidth, windowHeight));

  return ViewportPoint{(windowPosition.x - dest.x) / factor, (windowPosition.y - dest.y) / factor};
}

} // namespace arpg
