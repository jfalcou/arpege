#pragma once

#include <entt/signal/dispatcher.hpp>

namespace arpg
{

class screen_manager;
class pixel_canvas;

// Services reachable by a screen outside of its own world. Screens hold no
// pointer to each other and talk through the dispatcher.
struct app_context
{
  entt::dispatcher* events = nullptr;
  screen_manager* screens = nullptr;
  const pixel_canvas* canvas = nullptr;
  bool* quit_flag = nullptr;

  void request_quit() const
  {
    if (quit_flag != nullptr)
    {
      *quit_flag = true;
    }
  }
};

} // namespace arpg
