#pragma once

#include "core/AppContext.hpp"
#include "core/PixelCanvas.hpp"
#include "core/Screen.hpp"
#include "core/ScreenManager.hpp"

#include <entt/signal/dispatcher.hpp>

#include <memory>
#include <optional>
#include <string>

namespace arpg
{

struct AppConfig
{
  std::string title = "ARPG";
  int canvasWidth = 320;
  int canvasHeight = 180;
  int windowScale = 4;
  bool vsync = true;
  bool resizable = true;
};

// Window, game loop and shared services.
class Application
{
public:
  static constexpr float kFixedDt = 1.0f / 60.0f;
  static constexpr int kMaxStepsPerFrame = 5;

  explicit Application(AppConfig config = {});
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  // Runs until the window closes or the stack empties.
  void run(std::unique_ptr<Screen> initial);

private:
  void renderFrame(float alpha);
  void renderDevOverlay();

  AppConfig m_config;

  // Built after InitWindow: a render texture needs a live GL context.
  std::optional<PixelCanvas> m_canvas;

  ScreenManager m_screens;
  entt::dispatcher m_events;
  bool m_quit = false;
  bool m_devOverlay = false;
  float m_lastAlpha = 0.0f;
  int m_lastSteps = 0;
};

} // namespace arpg
