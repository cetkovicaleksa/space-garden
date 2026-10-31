#include "engine/Cursor.h"

#include "engine/Paths.h"

#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <iostream>
#include <string>

namespace engine {

namespace {

// Tip of the index finger in the 64x64 glove images (see tools/generate_assets.py).
constexpr int kHotspotX = 19;
constexpr int kHotspotY = 1;

GLFWcursor* loadCursor(const std::string& path)
{
    stbi_set_flip_vertically_on_load(false); // GLFW expects rows top to bottom
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!pixels) {
        std::cerr << "Failed to load cursor image: " << path << '\n';
        return nullptr;
    }
    GLFWimage image{w, h, pixels};
    GLFWcursor* cursor = glfwCreateCursor(&image, kHotspotX, kHotspotY);
    stbi_image_free(pixels);
    return cursor;
}

} // namespace

GloveCursor::GloveCursor(GLFWwindow* window)
    : window_(window)
    , open_(loadCursor(assetPath("cursor/glove_open.png")))
    , point_(loadCursor(assetPath("cursor/glove_point.png")))
{
    glfwSetCursor(window_, open_);
}

GloveCursor::~GloveCursor()
{
    glfwSetCursor(window_, nullptr);
    if (open_) glfwDestroyCursor(open_);
    if (point_) glfwDestroyCursor(point_);
}

void GloveCursor::setPressed(bool pressed)
{
    if (pressed == pressed_)
        return;
    pressed_ = pressed;
    glfwSetCursor(window_, pressed ? point_ : open_);
}

} // namespace engine
