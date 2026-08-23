#pragma once

#include "core/AppContext.hpp"
#include "core/PixelCanvas.hpp"
#include "core/Screen.hpp"
#include "core/ScreenManager.hpp"

#include <entt/signal/dispatcher.hpp>

#include <memory>
#include <optional>
#include <string>

namespace arpg {

struct AppConfig {
    std::string title = "ARPG";
    int canvasWidth = 320;     // resolution logique du monde
    int canvasHeight = 180;
    int windowScale = 4;       // taille de la fenetre au demarrage
    bool vsync = true;
    bool resizable = true;
};

// Fenetre, boucle de jeu et services partages.
class Application {
public:
    // Pas de simulation : 60 Hz, quoi qu'il arrive.
    static constexpr float kFixedDt = 1.0f / 60.0f;
    // Plafond de pas rattrapes par frame (anti spirale de la mort).
    static constexpr int kMaxStepsPerFrame = 5;

    explicit Application(AppConfig config = {});
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Prend la main jusqu'a la fermeture. `initial` est le premier ecran.
    void run(std::unique_ptr<Screen> initial);

private:
    void renderFrame(float alpha);
    void renderDevOverlay();

    AppConfig m_config;
    std::optional<PixelCanvas> m_canvas;  // construit apres InitWindow
    ScreenManager m_screens;
    entt::dispatcher m_events;
    bool m_quit = false;
    bool m_devOverlay = false;
    float m_lastAlpha = 0.0f;
    int m_lastSteps = 0;
};

} // namespace arpg
