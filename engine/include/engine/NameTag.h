#pragma once

#include "engine/Texture.h"

#include <glm/glm.hpp>

namespace engine {

class SpriteRenderer;

// Semi-transparent texture with the author's name and index, pinned to a screen corner.
class NameTag {
public:
    NameTag();
    // Draws in the coordinate space of the renderer's current projection.
    void draw(SpriteRenderer& sprites, glm::vec2 screenSize, float opacity = 0.55f) const;

private:
    Texture2D texture_;
};

} // namespace engine
