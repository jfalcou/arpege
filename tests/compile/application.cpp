// SPDX-License-Identifier: BSL-1.0

// Compile test: nothing here is executed. Constructing an application opens a
// window, so this only ever has to build and link.

#include "core/app_context.hpp"
#include "core/application.hpp"
#include "core/pixel_canvas.hpp"
#include "screens/main_menu_screen.hpp"

#include <memory>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<arpg::application>);
static_assert(!std::is_copy_constructible_v<arpg::pixel_canvas>);
static_assert(arpg::application::fixed_dt > 0.0f);
static_assert(arpg::application::max_steps_per_frame > 0);

// The context is a plain handle: screens receive it by value.
static_assert(std::is_trivially_copyable_v<arpg::app_context>);
static_assert(std::is_default_constructible_v<arpg::app_config>);

[[maybe_unused]] static void usage()
{
  arpg::app_config config;
  config.title = "arpg";
  config.canvas_width = 320;
  config.canvas_height = 180;
  config.window_scale = 4;
  config.vsync = true;
  config.resizable = true;

  arpg::application app(config);
  app.run(std::make_unique<arpg::main_menu_screen>());
}

[[maybe_unused]] static void canvas_usage()
{
  const arpg::pixel_canvas canvas(320, 180);

  canvas.begin_draw();
  canvas.end_draw();
  canvas.present();

  const Rectangle destination = canvas.destination();
  const Vector2 mouse = canvas.screen_to_canvas(Vector2{0.0f, 0.0f});

  (void)destination;
  (void)mouse;
  (void)canvas.scale();
  (void)canvas.target();
}

int main()
{
  return 0;
}
