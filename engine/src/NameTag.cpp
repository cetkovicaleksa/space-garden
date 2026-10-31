#include "engine/NameTag.h"

#include "engine/Paths.h"
#include "engine/SpriteRenderer.h"

namespace engine {

NameTag::NameTag()
    : texture_(assetPath("textures/name_tag.png"))
{
}

void NameTag::draw(SpriteRenderer& sprites, glm::vec2 screenSize, float opacity) const
{
    // Top-right corner, height ~8% of the screen.
    const float h = screenSize.y * 0.08f;
    const float w = h * float(texture_.width()) / float(texture_.height() > 0 ? texture_.height() : 1);
    const float margin = screenSize.y * 0.02f;
    sprites.draw(texture_, {screenSize.x - w - margin, screenSize.y - h - margin}, {w, h},
                 glm::vec4(1.0f, 1.0f, 1.0f, opacity));
}

} // namespace engine
