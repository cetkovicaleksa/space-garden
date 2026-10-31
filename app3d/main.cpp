// Space Garden 3D - entry point: owns the Garden, maps input to it, renders it in perspective.
#include "OrbitCamera.h"
#include "Renderer3D.h"
#include "Scene.h"

#include "engine/Cursor.h"
#include "engine/Window.h"

#include <garden/Garden.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

        OrbitCamera camera(scene::kGardenCenter, 3.9f, OrbitCamera::Limits{});
        Renderer3D renderer;

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

            renderer.draw(garden, {camera.view(), camera.projection(window.aspect()), camera.position(), float(window.time())});

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
