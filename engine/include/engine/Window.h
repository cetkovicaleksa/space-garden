#pragma once

#include <functional>

struct GLFWwindow;

namespace engine {

// Fullscreen GLFW window with an OpenGL 3.3 core context, Esc-to-exit and a frame limiter.
class Window {
public:
    using KeyCallback = std::function<void(int key, int scancode, int action, int mods)>;
    using MouseButtonCallback = std::function<void(int button, int action, int mods)>;

    explicit Window(const char* title, int targetFps = 75);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void close();

    // Call at the start of every frame. Polls events and returns the delta time in seconds.
    float beginFrame();
    // Swap buffers and sleep until the target frame time has passed.
    void endFrame();

    int width() const { return width_; }   // framebuffer size in pixels
    int height() const { return height_; }
    float aspect() const { return height_ > 0 ? float(width_) / float(height_) : 1.0f; }
    double time() const;
    float fps() const { return fps_; }

    // Cursor position in framebuffer pixels, origin top-left.
    void cursorPos(double& x, double& y) const;

    GLFWwindow* handle() const { return window_; }

    void setKeyCallback(KeyCallback cb) { keyCallback_ = std::move(cb); }
    void setMouseButtonCallback(MouseButtonCallback cb) { mouseButtonCallback_ = std::move(cb); }

private:
    static void onKey(GLFWwindow* w, int key, int scancode, int action, int mods);
    static void onMouseButton(GLFWwindow* w, int button, int action, int mods);
    static void onFramebufferSize(GLFWwindow* w, int width, int height);

    GLFWwindow* window_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    double frameDuration_ = 1.0 / 75.0;
    double frameStart_ = 0.0;
    double lastFrame_ = 0.0;
    float fps_ = 0.0f;
    int fpsFrames_ = 0;
    double fpsTimer_ = 0.0;

    KeyCallback keyCallback_;
    MouseButtonCallback mouseButtonCallback_;
};

} // namespace engine
