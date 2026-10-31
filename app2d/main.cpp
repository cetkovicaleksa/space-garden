// Space Garden 2D - entry point.
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

int main()
{
    try {
        engine::Window window("Space Garden 2D", 75);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        garden::Garden garden;

        engine::GloveCursor cursor(window.handle());
        engine::SpriteRenderer sprites;
        engine::NameTag nameTag;
        engine::InfoPanel panel;
        panel.setControls({"Left click / 1-3  select pot", "S  sow", "W  water", "H  harvest",
                           "L  lamp on/off", "R  refill tank", "Esc  quit"});

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
            default: break;
            }
        });

        while (!window.shouldClose()) {
            const float dt = window.beginFrame();
            garden.update(dt);
            panel.update(dt);

            glClearColor(0.05f, 0.07f, 0.16f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            const glm::vec2 screen(float(window.width()), float(window.height()));
            const glm::mat4 projection = glm::ortho(0.0f, screen.x, 0.0f, screen.y);
            sprites.setProjection(projection);
            panel.setProjection(projection);

            panel.draw(garden, sprites, {40.0f, 40.0f}, {screen.x - 80.0f, 260.0f});
            nameTag.draw(sprites, screen);

            window.endFrame();
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
