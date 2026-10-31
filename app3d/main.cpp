// Space Garden 3D - entry point: owns the Garden, maps input to it, renders it in perspective.
#include "OrbitCamera.h"
#include "Renderer3D.h"
#include "Scene.h"

#include "engine/Cursor.h"
#include "engine/InfoPanel.h"
#include "engine/NameTag.h"
#include "engine/SpriteRenderer.h"
#include "engine/Window.h"

#include <garden/Garden.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

// The HUD is laid out in a virtual space 1080 units high (width follows the aspect ratio).
constexpr float kHudHeight = 1080.0f;
constexpr float kPanelMargin = 24.0f;
constexpr float kPanelHeight = 250.0f;

std::string onOff(bool on) { return on ? "ON" : "OFF"; }

} // namespace

int main()
{
    try {
        engine::Window window("Space Garden 3D", 75);
        engine::GloveCursor cursor(window.handle());
        garden::Garden garden;

        OrbitCamera camera(scene::kGardenCenter, 3.9f, OrbitCamera::Limits{});
        Renderer3D renderer;

        // Fixed 2D overlay: info panel and name tag, drawn over the scene.
        engine::SpriteRenderer hud;
        engine::InfoPanel panel;
        engine::NameTag nameTag;
        panel.setControls({"Click / 1-3  select pot", "S sow   W water   H harvest", "L lamp   R refill tank",
                           "Arrows  move camera", "F1 depth test   F2 culling", "Esc  quit"});

        bool depthTest = true;
        bool faceCulling = true;

        window.setMouseButtonCallback([&](int button, int action, int) {
            if (button == GLFW_MOUSE_BUTTON_LEFT)
                cursor.setPressed(action == GLFW_PRESS);
        });
        window.setKeyCallback([&](int key, int, int action, int) {
            if (action != GLFW_PRESS)
                return;
            switch (key) {
            case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3: garden.select(key - GLFW_KEY_1); break;
            case GLFW_KEY_S: panel.notify(garden.apply(garden::Command::Sow)); break;
            case GLFW_KEY_W: panel.notify(garden.apply(garden::Command::Water)); break;
            case GLFW_KEY_H: panel.notify(garden.apply(garden::Command::Harvest)); break;
            case GLFW_KEY_L: panel.notify(garden.apply(garden::Command::ToggleLamp)); break;
            case GLFW_KEY_R: panel.notify(garden.apply(garden::Command::RefillTank)); break;
            case GLFW_KEY_F1: depthTest = !depthTest; break;
            case GLFW_KEY_F2: faceCulling = !faceCulling; break;
            default: break;
            }
        });

        while (!window.shouldClose()) {
            const float dt = window.beginFrame();
            GLFWwindow* w = window.handle();
            const float yaw = float(glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS) - float(glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS);
            const float up = float(glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS) - float(glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS);
            camera.update(yaw, up, dt);
            garden.update(dt);
            panel.update(dt);

            glViewport(0, 0, window.width(), window.height());
            glClearColor(0.03f, 0.04f, 0.08f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // The scene is framed above the info panel by shifting the image up (the camera still
            // looks straight at the garden center).
            const float lensShift = (kPanelMargin + kPanelHeight) / kHudHeight;
            if (depthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
            if (faceCulling) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);
            renderer.draw(garden, {camera.view(), camera.projection(window.aspect(), lensShift), camera.position(),
                                   float(window.time())});

            // HUD: no depth test or culling, alpha blending on.
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            const glm::vec2 hudSize(kHudHeight * window.aspect(), kHudHeight);
            const glm::mat4 ortho = glm::ortho(0.0f, hudSize.x, 0.0f, hudSize.y);
            hud.setProjection(ortho);
            panel.setProjection(ortho);
            panel.setStatusLines({"Depth test: " + onOff(depthTest) + "    Face culling: " + onOff(faceCulling)});
            panel.draw(garden, hud, {kPanelMargin, kPanelMargin}, {hudSize.x - 2.0f * kPanelMargin, kPanelHeight});
            nameTag.draw(hud, hudSize);

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
