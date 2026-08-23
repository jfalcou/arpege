#pragma once

#include <entt/signal/dispatcher.hpp>

namespace arpg
{

class ScreenManager;
class PixelCanvas;

// Services reachable by a screen outside of its own world. Screens hold no
// pointer to each other and talk through the dispatcher.
struct AppContext
{
  entt::dispatcher* events = nullptr;
  ScreenManager* screens = nullptr;
  const PixelCanvas* canvas = nullptr;
  bool* quitFlag = nullptr;

  void requestQuit() const
  {
    if (quitFlag != nullptr)
    {
      *quitFlag = true;
    }
  }
};

} // namespace arpg
