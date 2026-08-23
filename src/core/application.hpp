// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/action.hpp"
#include "core/app_context.hpp"
#include "core/input_snapshot.hpp"
#include "core/pixel_canvas.hpp"
#include "core/raylib_input.hpp"
#include "core/screen.hpp"
#include "core/screen_manager.hpp"

#include <entt/signal/dispatcher.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace arpg
{

struct app_config
{
  std::string title = "ARPG";
  int canvas_width = 320;
  int canvas_height = 180;
  int window_scale = 4;
  bool vsync = true;
  bool resizable = true;
};

// Window, game loop and shared services.
class application
{
public:
  static constexpr float fixed_dt = 1.0f / 60.0f;
  static constexpr int max_steps_per_frame = 5;

  explicit application(app_config config = {});
  ~application();

  application(const application&) = delete;
  application& operator=(const application&) = delete;

  // Runs until the window closes or the stack empties.
  void run(std::unique_ptr<screen> initial);

private:
  void sample_input();
  void render_frame(float alpha);
  void render_dev_overlay();

  app_config m_config;

  // Built after InitWindow: a render texture needs a live GL context.
  std::optional<pixel_canvas> m_canvas;

  screen_manager m_screens;
  entt::dispatcher m_events;

  raylib_input m_input_source;
  input_snapshot m_input;
  std::vector<binding> m_watched;
  bool m_quit = false;
  bool m_dev_overlay = false;
  float m_last_alpha = 0.0f;
  int m_last_steps = 0;
};

} // namespace arpg
