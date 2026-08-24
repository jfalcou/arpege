// SPDX-License-Identifier: BSL-1.0

#include <core/application.hpp>
#include <screens/main_menu_screen.hpp>

#include <memory>

int main()
{
  arpg::app_config config;
  config.title = "ARPG";
  config.canvas_width = 320;
  config.canvas_height = 180;
  config.window_scale = 4;

  arpg::application app(config);
  app.run(std::make_unique<arpg::main_menu_screen>());
  return 0;
}
