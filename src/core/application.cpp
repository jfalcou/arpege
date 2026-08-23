// SPDX-License-Identifier: BSL-1.0

#include "core/application.hpp"

#include "core/action_state.hpp"
#include "core/default_bindings.hpp"

#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

#include <algorithm>
#include <utility>

namespace arpg
{

application::application(app_config config)
  : m_config(std::move(config))
{
  unsigned int flags = FLAG_MSAA_4X_HINT;
  if (m_config.vsync)
  {
    flags |= FLAG_VSYNC_HINT;
  }
  if (m_config.resizable)
  {
    flags |= FLAG_WINDOW_RESIZABLE;
  }
  SetConfigFlags(flags);

  InitWindow(m_config.canvas_width * m_config.window_scale, m_config.canvas_height * m_config.window_scale,
             m_config.title.c_str());
  InitAudioDevice();

  // Escape must reach the screens instead of closing the window.
  SetExitKey(KEY_NULL);
  SetWindowMinSize(m_config.canvas_width, m_config.canvas_height);

  m_canvas.emplace(m_config.canvas_width, m_config.canvas_height);

  rlImGuiSetup(true);

  // Poll the union of the default bindings: what no map mentions cannot
  // trigger anything, so there is no point reading it.
  const control_codes codes = raylib_input::codes();
  for (const action_map& map : {dungeon_bindings(codes), menu_bindings(codes)})
  {
    for (const binding& control : map.controls())
    {
      if (std::find(m_watched.begin(), m_watched.end(), control) == m_watched.end())
      {
        m_watched.push_back(control);
      }
    }
  }

  m_screens.set_context(app_context{&m_events, &m_screens, &(*m_canvas), &m_quit, &m_input});
}

application::~application()
{
  m_screens.shutdown();
  rlImGuiShutdown();

  // The render texture must be released before the GL context dies.
  m_canvas.reset();

  if (IsAudioDeviceReady())
  {
    CloseAudioDevice();
  }
  CloseWindow();
}

void application::run(std::unique_ptr<screen> initial)
{
  m_screens.push(std::move(initial));
  m_screens.apply_pending();

  double accumulator = 0.0;

  while (!WindowShouldClose() && !m_quit && !m_screens.empty())
  {
    // Sampled once per rendered frame: every simulation step below sees the
    // same input, which is what keeps the fixed step deterministic.
    sample_input();

    double frame_time = static_cast<double>(GetFrameTime());
    if (frame_time > 0.25)
    {
      frame_time = 0.25;
    }
    accumulator += frame_time;

    int steps = 0;
    while (accumulator >= static_cast<double>(fixed_dt) && steps < max_steps_per_frame)
    {
      m_screens.update(fixed_dt);
      accumulator -= static_cast<double>(fixed_dt);
      ++steps;
    }

    // Dropping the backlog rather than carrying it over prevents the death
    // spiral when simulation cannot keep up.
    if (steps == max_steps_per_frame)
    {
      accumulator = 0.0;
    }
    m_last_steps = steps;

    m_last_alpha = static_cast<float>(accumulator / static_cast<double>(fixed_dt));
    render_frame(m_last_alpha);

    m_screens.apply_pending();
  }
}

void application::sample_input()
{
  const input_device previous = m_input.device;

  m_input_source.sample(m_watched, *m_canvas, m_input);
  m_input.device = active_device(m_input, previous);
}

void application::render_frame(float alpha)
{
  if (IsKeyPressed(KEY_F1))
  {
    m_dev_overlay = !m_dev_overlay;
  }

  m_canvas->begin_draw();
  m_screens.render(alpha);
  m_canvas->end_draw();

  BeginDrawing();
  ClearBackground(BLACK);
  m_canvas->present();

  // Drawn at native resolution, outside of the canvas, to keep the tooling
  // readable whatever the upscaling factor is.
  if (m_dev_overlay)
  {
    rlImGuiBegin();
    render_dev_overlay();
    rlImGuiEnd();
  }

  EndDrawing();
}

void application::render_dev_overlay()
{
  ImGui::SetNextWindowPos(ImVec2(8.0f, 8.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowBgAlpha(0.75f);

  if (ImGui::Begin("Debug (F1)"))
  {
    ImGui::Text("%d FPS  |  frame %.2f ms", GetFPS(), GetFrameTime() * 1000.0f);
    ImGui::Text("simulation steps this frame: %d", m_last_steps);
    ImGui::Text("interpolation alpha        : %.3f", static_cast<double>(m_last_alpha));
    ImGui::Separator();
    ImGui::Text("canvas %dx%d (x%d)", m_canvas->width(), m_canvas->height(), m_canvas->scale());
    ImGui::Text("window %dx%d", GetScreenWidth(), GetScreenHeight());
    ImGui::Text("screens on the stack: %zu", m_screens.size());
  }
  ImGui::End();
}

} // namespace arpg
