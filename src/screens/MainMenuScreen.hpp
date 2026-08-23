#pragma once

#include "core/Screen.hpp"

#include <raylib.h>

namespace arpg {

// Ecran de demarrage. Sert surtout de banc d'essai du socle : il verifie a
// l'oeil que le pas fixe et l'interpolation de rendu fonctionnent.
class MainMenuScreen : public Screen {
public:
    void onEnter() override;
    void update(float dt) override;
    void render(float alpha) override;

private:
    // Deux positions par entite : l'interpolation de rendu se fait entre
    // l'etat du pas precedent et celui du pas courant.
    Vector2 m_previous{};
    Vector2 m_current{};
    Vector2 m_velocity{};
    float m_radius = 4.0f;
};

} // namespace arpg
