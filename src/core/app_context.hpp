// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/input_snapshot.hpp"

#include <entt/signal/dispatcher.hpp>

namespace arpg
{

class screen_manager;
class pixel_canvas;

/// Services a screen may reach outside of its own world.
///
/// Screens hold no pointer to each other and talk through the dispatcher, so
/// none of them has to know which others exist.
struct app_context
{
  entt::dispatcher* events = nullptr;   ///< Event bus, how screens reach each other.
  screen_manager* screens = nullptr;    ///< The stack, to push or pop a screen.
  const pixel_canvas* canvas = nullptr; ///< Where the world is drawn.
  bool* quit_flag = nullptr;            ///< Raised by request_quit().

  /// Sampled once per rendered frame; every simulation step of that frame reads
  /// the same one. Screens resolve it through their own action_map.
  const input_snapshot* input = nullptr;

  /// Asks the application to leave the game loop at the end of the frame.
  void request_quit() const
  {
    if (quit_flag != nullptr)
    {
      *quit_flag = true;
    }
  }
};

} // namespace arpg
