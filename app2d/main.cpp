// Space Garden 2D - entry point.
#include "engine/Cursor.h"
#include "engine/NameTag.h"
#include "engine/Paths.h"
#include "engine/SpriteRenderer.h"
#include "engine/TextRenderer.h"
#include "engine/Window.h"

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

        engine::GloveCursor cursor(window.handle());
        window.setMouseButtonCallback([&](int button, int action, int) {
            if (button == GLFW_MOUSE_BUTTON_LEFT)
                cursor.setPressed(action == GLFW_PRESS);
        });

        engine::SpriteRenderer sprites;
        engine::NameTag nameTag;
        engine::TextRenderer text(engine::assetPath("fonts/DejaVuSans.ttf"), 48);

        while (!window.shouldClose()) {
            window.beginFrame();

            glClearColor(0.05f, 0.07f, 0.16f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            const glm::vec2 screen(float(window.width()), float(window.height()));
            sprites.setProjection(glm::ortho(0.0f, screen.x, 0.0f, screen.y));

            // A test rectangle under the name tag to show that the tag is see-through.
            sprites.drawRect({screen.x * 0.55f, screen.y * 0.80f}, {screen.x * 0.4f, screen.y * 0.18f},
                             {0.35f, 0.75f, 0.45f, 1.0f});
            nameTag.draw(sprites, screen);

            // Text test: several sizes and colors.
            text.setProjection(glm::ortho(0.0f, screen.x, 0.0f, screen.y));
            text.draw("Space Garden", 60.0f, screen.y * 0.5f, 1.5f, {1.0f, 1.0f, 1.0f, 1.0f});
            text.draw("Moisture 75%  Growth 33%", 60.0f, screen.y * 0.5f - 80.0f, 0.8f, {0.5f, 0.8f, 1.0f, 1.0f});
            text.draw("Aleksa Ćetković - čćšžđ", 60.0f, screen.y * 0.5f - 140.0f, 0.6f, {1.0f, 0.8f, 0.3f, 0.8f});

            window.endFrame();
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
