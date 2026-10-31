// Space Garden 2D - entry point.
#include "engine/Window.h"

#include <glad/glad.h>

#include <cstdlib>
#include <exception>
#include <iostream>

int main()
{
    try {
        engine::Window window("Space Garden 2D", 75);

        double lastFpsPrint = window.time();
        while (!window.shouldClose()) {
            window.beginFrame();

            glClearColor(0.05f, 0.07f, 0.16f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Temporary: print the measured frame rate once per second.
            if (window.time() - lastFpsPrint >= 1.0) {
                std::cout << "FPS: " << window.fps() << std::endl;
                lastFpsPrint = window.time();
            }

            window.endFrame();
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
