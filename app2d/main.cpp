// Space Garden 2D - entry point: owns the Garden, maps input to it, renders it.
#include "Renderer2D.h"

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
        engine::Window window("Space Garden 2D", 75);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        garden::Garden garden;
        engine::GloveCursor cursor(window.handle());
        Renderer2D renderer;
        renderer.panel().setControls({"Left click / 1-3  select pot", "S  sow", "W  water", "H  harvest",
                                      "L  lamp on/off", "R  refill tank", "Esc  quit"});

        window.setMouseButtonCallback([&](int button, int action, int) {
            if (button != GLFW_MOUSE_BUTTON_LEFT)
                return;
            cursor.setPressed(action == GLFW_PRESS);
            if (action == GLFW_PRESS) {
                double x = 0.0, y = 0.0;
                window.cursorPos(x, y);
                // Clicking outside the pots keeps the current selection.
                const int pot = renderer.pickPot(renderer.toVirtual(x, y));
                if (pot >= 0)
                    garden.select(pot);
            }
        });
        window.setKeyCallback([&](int key, int, int action, int) {
            if (action != GLFW_PRESS)
                return;
            switch (key) {
            case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3: garden.select(key - GLFW_KEY_1); break;
            case GLFW_KEY_S: renderer.panel().notify(garden.apply(garden::Command::Sow)); break;
            case GLFW_KEY_W: renderer.panel().notify(garden.apply(garden::Command::Water)); break;
            case GLFW_KEY_H: renderer.panel().notify(garden.apply(garden::Command::Harvest)); break;
            case GLFW_KEY_L: renderer.panel().notify(garden.apply(garden::Command::ToggleLamp)); break;
            case GLFW_KEY_R: renderer.panel().notify(garden.apply(garden::Command::RefillTank)); break;
            default: break;
            }
        });

        while (!window.shouldClose()) {
            const float dt = window.beginFrame();
            garden.update(dt);
            renderer.panel().update(dt);

            renderer.beginFrame(window.width(), window.height());
            double mx = 0.0, my = 0.0;
            window.cursorPos(mx, my);
            renderer.setHovered(renderer.pickPot(renderer.toVirtual(mx, my)));
            renderer.draw(garden, float(window.time()));

            window.endFrame();
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
#ifdef _WIN32
        MessageBoxA(nullptr, e.what(), "Space Garden 2D", MB_OK | MB_ICONERROR); // no console in release builds
#endif
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
