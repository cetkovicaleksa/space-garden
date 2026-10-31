#pragma once

#include "Layout.h"

#include "engine/InfoPanel.h"
#include "engine/NameTag.h"
#include "engine/SpriteRenderer.h"
#include "engine/TextRenderer.h"
#include "engine/Texture.h"

#include <garden/Garden.h>

#include <glm/glm.hpp>

// Orthographic renderer of the Garden. Only reads the model; never changes it.
class Renderer2D {
public:
    Renderer2D();

    // Letterbox the virtual canvas into the framebuffer; call once per frame before draw().
    void beginFrame(int framebufferWidth, int framebufferHeight);
    void draw(const garden::Garden& garden, float time);

    // Framebuffer pixel (origin top-left) -> virtual canvas coordinates (origin bottom-left).
    glm::vec2 toVirtual(double px, double py) const;
    // Index of the pot under a virtual-space point, or -1.
    int pickPot(glm::vec2 p) const;

    engine::InfoPanel& panel() { return panel_; }

private:
    void drawCabinet();
    void drawPots(const garden::Garden& garden, float time);

    engine::SpriteRenderer sprites_;
    engine::TextRenderer title_;
    engine::InfoPanel panel_;
    engine::NameTag nameTag_;

    engine::Texture2D space_, metal_, wall_, pot_, soil_;
    engine::Texture2D seed_, sprout_, mature_;
    engine::Texture2D lamp_, beam_, tank_, tankGlass_, droplet_;

    glm::vec2 viewportOffset_{0.0f};
    float viewportScale_ = 1.0f;
};
