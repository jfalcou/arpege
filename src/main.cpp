#include "core/Application.hpp"
#include "screens/MainMenuScreen.hpp"

#include <memory>

int main()
{
    arpg::AppConfig config;
    config.title = "ARPG";
    config.canvasWidth = 320;
    config.canvasHeight = 180;
    config.windowScale = 4;

    arpg::Application app(config);
    app.run(std::make_unique<arpg::MainMenuScreen>());
    return 0;
}
