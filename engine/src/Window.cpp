#include "engine/Window.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <timeapi.h>
#endif

namespace engine {

namespace {
void glfwErrorCallback(int code, const char* description)
{
    std::cerr << "GLFW error " << code << ": " << description << '\n';
}
} // namespace

Window::Window(const char* title, int targetFps)
    : frameDuration_(1.0 / double(targetFps > 0 ? targetFps : 75))
{
#ifdef _WIN32
    timeBeginPeriod(1); // 1 ms sleep granularity, otherwise the 75 FPS limiter oversleeps
#endif
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4);

    // Fullscreen on the primary monitor at its current video mode.
    // SPACE_GARDEN_WINDOWED=1 opens a 1280x720 window instead (debugging only).
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
    const char* windowedEnv = std::getenv("SPACE_GARDEN_WINDOWED");
    const bool windowed = (windowedEnv && windowedEnv[0] == '1') || !mode;

    if (windowed) {
        window_ = glfwCreateWindow(1280, 720, title, nullptr, nullptr);
    } else {
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        window_ = glfwCreateWindow(mode->width, mode->height, title, monitor, nullptr);
    }
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window_);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD");
    }

    // VSync off: the frame limiter decides the frame rate, not the monitor.
    glfwSwapInterval(0);

    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, onKey);
    glfwSetMouseButtonCallback(window_, onMouseButton);
    glfwSetFramebufferSizeCallback(window_, onFramebufferSize);

    glfwGetFramebufferSize(window_, &width_, &height_);
    glViewport(0, 0, width_, height_);

    std::cout << "OpenGL " << glGetString(GL_VERSION) << " | " << glGetString(GL_RENDERER) << '\n';

    lastFrame_ = frameStart_ = fpsTimer_ = glfwGetTime();
}

Window::~Window()
{
    if (window_)
        glfwDestroyWindow(window_);
    glfwTerminate();
#ifdef _WIN32
    timeEndPeriod(1);
#endif
}

bool Window::shouldClose() const { return glfwWindowShouldClose(window_); }

void Window::close() { glfwSetWindowShouldClose(window_, GLFW_TRUE); }

double Window::time() const { return glfwGetTime(); }

void Window::cursorPos(double& x, double& y) const
{
    double cx = 0.0, cy = 0.0;
    glfwGetCursorPos(window_, &cx, &cy);
    // Convert from screen coordinates to framebuffer pixels (differs on HiDPI displays).
    int ww = 1, wh = 1;
    glfwGetWindowSize(window_, &ww, &wh);
    x = cx * (ww > 0 ? double(width_) / ww : 1.0);
    y = cy * (wh > 0 ? double(height_) / wh : 1.0);
}

float Window::beginFrame()
{
    glfwPollEvents();
    frameStart_ = glfwGetTime();
    float dt = float(frameStart_ - lastFrame_);
    lastFrame_ = frameStart_;
    if (dt > 0.1f)
        dt = 0.1f; // avoid huge jumps after a stall (window drag, breakpoint)

    ++fpsFrames_;
    if (frameStart_ - fpsTimer_ >= 1.0) {
        fps_ = float(fpsFrames_ / (frameStart_ - fpsTimer_));
        fpsFrames_ = 0;
        fpsTimer_ = frameStart_;
    }
    return dt;
}

void Window::endFrame()
{
    glfwSwapBuffers(window_);

    // Frame limiter: sleep for most of the remaining time, then spin for precision.
    const double target = frameStart_ + frameDuration_;
    double now = glfwGetTime();
    if (target - now > 0.002)
        std::this_thread::sleep_for(std::chrono::duration<double>(target - now - 0.0015));
    while (glfwGetTime() < target) {
        std::this_thread::yield();
    }
}

void Window::onKey(GLFWwindow* w, int key, int scancode, int action, int mods)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(w, GLFW_TRUE); // Esc always exits
        return;
    }
    if (self && self->keyCallback_)
        self->keyCallback_(key, scancode, action, mods);
}

void Window::onMouseButton(GLFWwindow* w, int button, int action, int mods)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (self && self->mouseButtonCallback_)
        self->mouseButtonCallback_(button, action, mods);
}

void Window::onFramebufferSize(GLFWwindow* w, int width, int height)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (self) {
        self->width_ = width;
        self->height_ = height;
    }
    glViewport(0, 0, width, height);
}

} // namespace engine
