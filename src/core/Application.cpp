#include "core/Application.hpp"

#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

#include <utility>

namespace arpg {

Application::Application(AppConfig config)
    : m_config(std::move(config))
{
    unsigned int flags = FLAG_MSAA_4X_HINT;
    if (m_config.vsync) {
        flags |= FLAG_VSYNC_HINT;
    }
    if (m_config.resizable) {
        flags |= FLAG_WINDOW_RESIZABLE;
    }
    SetConfigFlags(flags);

    InitWindow(
        m_config.canvasWidth * m_config.windowScale,
        m_config.canvasHeight * m_config.windowScale,
        m_config.title.c_str()
    );
    InitAudioDevice();

    // Echap ne ferme pas le jeu : en donjon, c'est la pause.
    SetExitKey(KEY_NULL);
    SetWindowMinSize(m_config.canvasWidth, m_config.canvasHeight);

    m_canvas.emplace(m_config.canvasWidth, m_config.canvasHeight);

    rlImGuiSetup(true);

    m_screens.setContext(AppContext{&m_events, &m_screens, &(*m_canvas), &m_quit});
}

Application::~Application()
{
    m_screens.shutdown();
    rlImGuiShutdown();
    m_canvas.reset();
    if (IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
    CloseWindow();
}

void Application::run(std::unique_ptr<Screen> initial)
{
    m_screens.push(std::move(initial));
    m_screens.applyPending();

    double accumulator = 0.0;

    while (!WindowShouldClose() && !m_quit && !m_screens.empty()) {
        // TODO(input) : capturer ici l'etat des actions, une fois par frame,
        // et le faire consommer par les pas de simulation ci-dessous.

        double frameTime = static_cast<double>(GetFrameTime());
        if (frameTime > 0.25) {
            frameTime = 0.25;  // un breakpoint ne doit pas rattraper 10 s
        }
        accumulator += frameTime;

        int steps = 0;
        while (accumulator >= static_cast<double>(kFixedDt) && steps < kMaxStepsPerFrame) {
            m_screens.update(kFixedDt);
            accumulator -= static_cast<double>(kFixedDt);
            ++steps;
        }
        if (steps == kMaxStepsPerFrame) {
            accumulator = 0.0;  // on renonce au retard plutot que de spiraler
        }
        m_lastSteps = steps;

        m_lastAlpha = static_cast<float>(accumulator / static_cast<double>(kFixedDt));
        renderFrame(m_lastAlpha);

        // Les changements d'ecran ne prennent effet qu'ici, hors update.
        m_screens.applyPending();
    }
}

void Application::renderFrame(float alpha)
{
    if (IsKeyPressed(KEY_F1)) {
        m_devOverlay = !m_devOverlay;
    }

    // 1. le monde, en basse resolution
    m_canvas->beginDraw();
    m_screens.render(alpha);
    m_canvas->endDraw();

    // 2. agrandissement vers la fenetre
    BeginDrawing();
    ClearBackground(BLACK);
    m_canvas->present();

    // 3. outils de dev, en resolution native (jamais dans le canvas)
    if (m_devOverlay) {
        rlImGuiBegin();
        renderDevOverlay();
        rlImGuiEnd();
    }

    EndDrawing();
}

void Application::renderDevOverlay()
{
    ImGui::SetNextWindowPos(ImVec2(8.0f, 8.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.75f);
    if (ImGui::Begin("Debug (F1)")) {
        ImGui::Text("%d FPS  |  frame %.2f ms", GetFPS(), GetFrameTime() * 1000.0f);
        ImGui::Text("pas de simu cette frame : %d", m_lastSteps);
        ImGui::Text("alpha interpolation     : %.3f", static_cast<double>(m_lastAlpha));
        ImGui::Separator();
        ImGui::Text("canvas %dx%d  (x%d)", m_canvas->width(), m_canvas->height(), m_canvas->scale());
        ImGui::Text("fenetre %dx%d", GetScreenWidth(), GetScreenHeight());
        ImGui::Text("ecrans empiles : %zu", m_screens.size());
    }
    ImGui::End();
}

} // namespace arpg
