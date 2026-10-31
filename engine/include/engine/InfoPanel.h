#pragma once

#include "engine/TextRenderer.h"

#include <garden/Garden.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace engine {

class SpriteRenderer;

// Information panel shared by both apps: stage, moisture/growth/water bars with percentages,
// lamp state, a controls reminder, warnings and feedback for rejected commands.
// Drawn into a rectangle given in the current projection's units (origin bottom-left, y up).
class InfoPanel {
public:
    InfoPanel();

    void setControls(std::vector<std::string> lines) { controls_ = std::move(lines); }
    // Extra status lines shown under the lamp state (e.g. depth test / culling in 3D).
    void setStatusLines(std::vector<std::string> lines) { status_ = std::move(lines); }

    // Show feedback for a command result (only failures produce a visible message).
    void notify(garden::CommandResult result);
    void update(float dt);

    void setProjection(const glm::mat4& projection);
    void draw(const garden::Garden& garden, SpriteRenderer& sprites, glm::vec2 pos, glm::vec2 size);

private:
    void drawBar(SpriteRenderer& sprites, const std::string& label, float ratio, bool enabled,
                 glm::vec2 pos, glm::vec2 size, glm::vec4 color);

    TextRenderer text_;
    TextRenderer bold_;
    std::vector<std::string> controls_;
    std::vector<std::string> status_;
    std::string message_;
    float messageTimer_ = 0.0f;
};

} // namespace engine
