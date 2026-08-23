#pragma once

#include "core/AppContext.hpp"
#include "core/Screen.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace arpg {

// Pile d'ecrans. Les demandes de changement sont mises en file et appliquees
// en fin de frame (applyPending) : jamais au milieu d'un update, sinon on
// detruirait un ecran en train de s'executer.
class ScreenManager {
public:
    void setContext(const AppContext& context) { m_context = context; }

    void push(std::unique_ptr<Screen> screen);
    void pop();
    void replace(std::unique_ptr<Screen> screen);
    void clear();

    // Applique les demandes accumulees. A appeler une fois par frame,
    // apres update et render.
    void applyPending();

    // Met a jour les ecrans du haut de la pile jusqu'au premier qui bloque.
    void update(float dt);
    // Dessine a partir du dernier ecran opaque, du fond vers le sommet.
    void render(float alpha);

    // Vide la pile immediatement (onExit appele), pour l'arret du programme.
    void shutdown();

    bool empty() const { return m_stack.empty(); }
    std::size_t size() const { return m_stack.size(); }
    Screen* top() { return m_stack.empty() ? nullptr : m_stack.back().get(); }

private:
    enum class CommandKind { Push, Pop, Replace, Clear };

    struct Command {
        CommandKind kind;
        std::unique_ptr<Screen> screen;
    };

    void enter(std::unique_ptr<Screen> screen);
    void leaveTop();

    std::vector<std::unique_ptr<Screen>> m_stack;
    std::vector<Command> m_pending;
    AppContext m_context{};
};

} // namespace arpg
