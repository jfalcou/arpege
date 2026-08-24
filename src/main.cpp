// SPDX-License-Identifier: BSL-1.0

#include <core/application.hpp>
#include <core/command_line.hpp>
#include <screens/main_menu_screen.hpp>

#include <cstdio>
#include <memory>
#include <string_view>
#include <vector>

int main(int argc, char** argv)
{
  // The program name is not an option.
  const std::vector<std::string_view> arguments(argv + 1, argv + argc);
  const arpg::launch_options options = arpg::parse_command_line(arguments);

  if (!options.valid())
  {
    std::fprintf(stderr, "arpg: %s\n\n%s", options.error.c_str(), arpg::usage().data());
    return 1;
  }

  if (options.help)
  {
    std::fprintf(stdout, "%s", arpg::usage().data());
    return 0;
  }

  arpg::app_config config;
  config.title = "ARPG";
  config.canvas_width = 320;
  config.canvas_height = 180;
  config.window_scale = 4;
  config.assets = options.assets;

  arpg::application app(config);
  app.run(std::make_unique<arpg::main_menu_screen>());
  return 0;
}
