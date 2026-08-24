// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/action.hpp>
#include <core/app_context.hpp>
#include <core/input_snapshot.hpp>
#include <core/pixel_canvas.hpp>
#include <core/raylib_input.hpp>
#include <core/screen.hpp>
#include <core/screen_manager.hpp>

#include <entt/signal/dispatcher.hpp>

#include <filesystem>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace arpg
{

/// How the window and the canvas are set up.
struct app_config
{
  std::string title = "ARPG"; ///< Window title.
  int canvas_width = 320;     ///< Logical width of the world, in pixels.
  int canvas_height = 180;    ///< Logical height of the world, in pixels.
  int window_scale = 4;       ///< Window size at startup, as a multiple of the canvas.
  bool vsync = true;          ///< Cap the render rate to the refresh rate.
  bool resizable = true;      ///< Let the window be resized; the canvas rescales to fit.
};

/// Window, game loop and shared services.
///
/// Owns the fixed timestep loop: the simulation advances in steps of
/// #fixed_dt while rendering happens at whatever rate the screen runs at.
class application
{
public:
  /// Length of a simulation step. Fixed, so a replay of the same inputs gives
  /// the same game.
  static constexpr float fixed_dt = 1.0f / 60.0f;

  /// Most steps caught up in one frame, past which the backlog is dropped
  /// rather than chased, which would spiral.
  static constexpr int max_steps_per_frame = 5;

  /// Opens the window and the audio device, and sets the tooling up.
  explicit application(app_config config = {});
  /// Tears everything down in the order the backend expects.
  ~application();

  application(const application&) = delete;
  application& operator=(const application&) = delete;

  /// Runs until the window closes or the screen stack empties.
  ///
  /// @param initial the first screen, stacked before the loop starts.
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

  /// Outlives the context that points at it.
  std::filesystem::path m_assets;

  raylib_input m_input_source;
  input_snapshot m_input;
  std::vector<binding> m_watched;
  bool m_quit = false;
  bool m_dev_overlay = false;
  float m_last_alpha = 0.0f;
  int m_last_steps = 0;
};

} // namespace arpg
