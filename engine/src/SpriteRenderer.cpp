#include "engine/SpriteRenderer.h"

#include "engine/Paths.h"
#include "engine/Texture.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace engine {

SpriteRenderer::SpriteRenderer()
    : shader_(shaderPath("sprite.vs"), shaderPath("sprite.fs"))
{
    // Unit quad [0,1]x[0,1]: position (x, y) and UV (u, v) interleaved.
    const float vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
    };
    const unsigned int indices[] = {0, 1, 2, 2, 3, 0};

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    shader_.use();
    shader_.setInt("uImage", 0);
    shader_.setVec2("uUvScale", 1.0f, 1.0f);
}

SpriteRenderer::~SpriteRenderer()
{
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ebo_);
}

void SpriteRenderer::setProjection(const glm::mat4& projection)
{
    shader_.use();
    shader_.setMat4("uProjection", projection);
}

void SpriteRenderer::submit(glm::vec2 pos, glm::vec2 size, float rotationDeg)
{
    // model = T * R * S (scale first); rotation is around the sprite center.
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(pos, 0.0f));
    if (rotationDeg != 0.0f) {
        model = glm::translate(model, glm::vec3(0.5f * size, 0.0f));
        model = glm::rotate(model, glm::radians(rotationDeg), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-0.5f * size, 0.0f));
    }
    model = glm::scale(model, glm::vec3(size, 1.0f));
    shader_.setMat4("uModel", model);

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void SpriteRenderer::draw(const Texture2D& texture, glm::vec2 pos, glm::vec2 size, glm::vec4 tint, float rotationDeg)
{
    shader_.use();
    shader_.setInt("uMode", 1);
    shader_.setVec4("uColor", tint);
    texture.bind(0);
    submit(pos, size, rotationDeg);
}

void SpriteRenderer::drawTiled(const Texture2D& texture, glm::vec2 pos, glm::vec2 size, glm::vec2 tileSize,
                               glm::vec4 tint)
{
    shader_.use();
    shader_.setInt("uMode", 1);
    shader_.setVec4("uColor", tint);
    shader_.setVec2("uUvScale", size / tileSize);
    texture.bind(0);
    submit(pos, size, 0.0f);
    shader_.setVec2("uUvScale", 1.0f, 1.0f);
}

void SpriteRenderer::drawRect(glm::vec2 pos, glm::vec2 size, glm::vec4 color, float rotationDeg)
{
    shader_.use();
    shader_.setInt("uMode", 0);
    shader_.setVec4("uColor", color);
    submit(pos, size, rotationDeg);
}

void SpriteRenderer::drawFrame(glm::vec2 pos, glm::vec2 size, float t, glm::vec4 color)
{
    drawRect(pos, {size.x, t}, color);
    drawRect({pos.x, pos.y + size.y - t}, {size.x, t}, color);
    drawRect({pos.x, pos.y + t}, {t, size.y - 2.0f * t}, color);
    drawRect({pos.x + size.x - t, pos.y + t}, {t, size.y - 2.0f * t}, color);
}

void SpriteRenderer::drawGradient(glm::vec2 pos, glm::vec2 size, glm::vec4 bottom, glm::vec4 top)
{
    shader_.use();
    shader_.setInt("uMode", 2);
    shader_.setVec4("uColor", bottom);
    shader_.setVec4("uColor2", top);
    submit(pos, size, 0.0f);
}

} // namespace engine
