#pragma once

#include <raylib.h>

namespace arpg {

// Cible de rendu basse resolution du monde. Tout le jeu se dessine dedans
// en pixels "logiques", puis on l'agrandit d'un facteur entier vers la
// fenetre (nearest neighbor) pour garder des pixels carres et nets.
class PixelCanvas {
public:
    PixelCanvas(int width, int height);
    ~PixelCanvas();

    PixelCanvas(const PixelCanvas&) = delete;
    PixelCanvas& operator=(const PixelCanvas&) = delete;

    // Encadre le dessin du monde en basse resolution.
    void beginDraw(Color clear = BLACK) const;
    void endDraw() const;

    // Agrandit le canvas vers la fenetre, centre, avec letterbox.
    void present() const;

    // Convertit une position fenetre (souris) en pixel du canvas.
    Vector2 screenToCanvas(Vector2 screenPosition) const;

    int width() const { return m_width; }
    int height() const { return m_height; }
    const RenderTexture2D& target() const { return m_target; }

    // Zone de la fenetre effectivement occupee par le canvas.
    Rectangle destination() const;
    // Facteur d'agrandissement entier courant (>= 1).
    int scale() const;

private:
    int m_width;
    int m_height;
    RenderTexture2D m_target{};
};

} // namespace arpg
