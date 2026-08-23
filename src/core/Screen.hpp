#pragma once

#include "core/AppContext.hpp"

namespace arpg {

// Un mode de jeu (menu, carte roguelike, donjon, pause...).
// Seule couche du moteur ou le virtual est autorise : c'est de
// l'orchestration, appelee quelques fois par frame, jamais dans une
// boucle chaude.
class Screen {
public:
    virtual ~Screen() = default;

    Screen() = default;
    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;

    // Appeles par le ScreenManager quand l'ecran entre / sort de la pile.
    // C'est la que se chargent et se liberent les ressources de l'ecran.
    virtual void onEnter() {}
    virtual void onExit() {}

    // Simulation a pas fixe : dt vaut toujours Application::kFixedDt.
    virtual void update(float dt) = 0;

    // Rendu a la frequence de l'ecran. `alpha` est le reliquat de
    // l'accumulateur dans [0, 1[ : interpoler entre l'etat precedent et
    // l'etat courant pour un mouvement lisse.
    virtual void render(float alpha) = 0;

    // false : l'ecran en dessous continue de tourner / d'etre dessine.
    // Une pause bloque l'update mais pas le rendu ; une transition ne
    // bloque generalement ni l'un ni l'autre.
    virtual bool blocksUpdate() const { return true; }
    virtual bool blocksRender() const { return true; }

    void attach(const AppContext& context) { m_context = context; }

protected:
    const AppContext& ctx() const { return m_context; }

private:
    AppContext m_context{};
};

} // namespace arpg
