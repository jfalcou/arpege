#pragma once

#include <entt/signal/dispatcher.hpp>

namespace arpg {

class ScreenManager;
class PixelCanvas;

// Poignee passee a chaque ecran : tout ce qu'un ecran a le droit de toucher
// en dehors de son propre monde. Les ecrans ne se connaissent pas entre eux,
// ils communiquent par le dispatcher.
struct AppContext {
    entt::dispatcher* events = nullptr;
    ScreenManager* screens = nullptr;
    const PixelCanvas* canvas = nullptr;
    bool* quitFlag = nullptr;

    void requestQuit() const {
        if (quitFlag != nullptr) {
            *quitFlag = true;
        }
    }
};

} // namespace arpg
