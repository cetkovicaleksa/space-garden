#pragma once

struct GLFWwindow;
struct GLFWcursor;

namespace engine {

// Themed OS cursor: an astronaut glove that points its index finger while the left button is held.
// Implemented with glfwCreateCursor (crisp, no extra render pass, hotspot at the fingertip).
class GloveCursor {
public:
    explicit GloveCursor(GLFWwindow* window);
    ~GloveCursor();

    GloveCursor(const GloveCursor&) = delete;
    GloveCursor& operator=(const GloveCursor&) = delete;

    void setPressed(bool pressed);
    bool pressed() const { return pressed_; }

private:
    GLFWwindow* window_;
    GLFWcursor* open_ = nullptr;
    GLFWcursor* point_ = nullptr;
    bool pressed_ = false;
};

} // namespace engine
