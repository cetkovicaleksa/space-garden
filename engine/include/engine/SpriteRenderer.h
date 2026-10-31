#pragma once

#include <learnopengl/shader.h>

#include <glm/glm.hpp>

namespace engine {

class Texture2D;

// Draws textured or flat-colored quads in 2D (LearnOpenGL "Breakout" style):
// one unit-quad VAO, a model matrix per sprite. Coordinates follow the current
// orthographic projection (origin bottom-left, y up). `pos` is the bottom-left corner.
class SpriteRenderer {
public:
    SpriteRenderer();
    ~SpriteRenderer();

    SpriteRenderer(const SpriteRenderer&) = delete;
    SpriteRenderer& operator=(const SpriteRenderer&) = delete;

    void setProjection(const glm::mat4& projection);

    void draw(const Texture2D& texture, glm::vec2 pos, glm::vec2 size,
              glm::vec4 tint = glm::vec4(1.0f), float rotationDeg = 0.0f);
    // Texture repeated every `tileSize` units (texture must use GL_REPEAT wrapping).
    void drawTiled(const Texture2D& texture, glm::vec2 pos, glm::vec2 size, glm::vec2 tileSize,
                   glm::vec4 tint = glm::vec4(1.0f));
    void drawRect(glm::vec2 pos, glm::vec2 size, glm::vec4 color, float rotationDeg = 0.0f);
    // Rectangle outline made of four thin rectangles.
    void drawFrame(glm::vec2 pos, glm::vec2 size, float thickness, glm::vec4 color);
    // Vertical gradient (bottom color -> top color), used for the lamp beam.
    void drawGradient(glm::vec2 pos, glm::vec2 size, glm::vec4 bottom, glm::vec4 top);

private:
    void submit(glm::vec2 pos, glm::vec2 size, float rotationDeg);

    Shader shader_;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int ebo_ = 0;
};

} // namespace engine
