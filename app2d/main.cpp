// Space Garden 2D - entry point.
#include "engine/Paths.h"
#include "engine/Window.h"

#include <glad/glad.h>
#include <learnopengl/shader.h>

#include <cstdlib>
#include <exception>
#include <iostream>

int main()
{
    try {
        engine::Window window("Space Garden 2D", 75);

        Shader colorShader(engine::shaderPath("color.vs"), engine::shaderPath("color.fs"));

        // One rectangle in normalized device coordinates: 4 vertices + 6 indices.
        const float vertices[] = {
            -0.5f, -0.3f,
             0.5f, -0.3f,
             0.5f,  0.3f,
            -0.5f,  0.3f,
        };
        const unsigned int indices[] = {0, 1, 2, 2, 3, 0};

        unsigned int vao = 0, vbo = 0, ebo = 0;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        while (!window.shouldClose()) {
            window.beginFrame();

            glClearColor(0.05f, 0.07f, 0.16f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            colorShader.use();
            colorShader.setVec4("uColor", 0.35f, 0.75f, 0.45f, 1.0f);
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

            window.endFrame();
        }

        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
