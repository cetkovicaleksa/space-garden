// Space Garden 3D - entry point: owns the Garden, maps input to it, renders it in perspective.
#include "GpuMesh.h"
#include "OrbitCamera.h"
#include "Primitives.h"

#include "engine/Cursor.h"
#include "engine/Paths.h"
#include "engine/Window.h"

#include <garden/Garden.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <learnopengl/shader.h>

#include <cstdlib>
#include <exception>
#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

int main()
{
    try {
        engine::Window window("Space Garden 3D", 75);
        engine::GloveCursor cursor(window.handle());
        garden::Garden garden;

        OrbitCamera camera({0.0f, 0.5f, 0.0f}, 3.5f, OrbitCamera::Limits{});
        Shader shader(engine::shaderPath("scene.vs"), engine::shaderPath("scene.fs"));
        GpuMesh cube(prim::box({-0.5f, 0.0f, -0.5f}, {0.5f, 1.0f, 0.5f}));

        glEnable(GL_DEPTH_TEST);
        window.setMouseButtonCallback([&](int button, int action, int) {
            if (button == GLFW_MOUSE_BUTTON_LEFT)
                cursor.setPressed(action == GLFW_PRESS);
        });

        while (!window.shouldClose()) {
            const float dt = window.beginFrame();
            GLFWwindow* w = window.handle();
            const float yaw = float(glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS) - float(glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS);
            const float up = float(glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS) - float(glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS);
            camera.update(yaw, up, dt);
            garden.update(dt);

            glViewport(0, 0, window.width(), window.height());
            glClearColor(0.03f, 0.04f, 0.08f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            shader.use();
            shader.setMat4("uView", camera.view());
            shader.setMat4("uProjection", camera.projection(window.aspect()));
            shader.setMat4("uModel", glm::mat4(1.0f));
            shader.setMat3("uNormalMatrix", glm::mat3(1.0f));
            shader.setVec3("uColor", glm::vec3(0.4f, 0.7f, 1.0f));
            cube.draw();

            window.endFrame();
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
#ifdef _WIN32
        MessageBoxA(nullptr, e.what(), "Space Garden 3D", MB_OK | MB_ICONERROR);
#endif
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
