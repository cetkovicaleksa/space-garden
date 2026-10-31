#pragma once

#include <glm/glm.hpp>

// Perspective camera that moves on a limited arc in front of the cabinet and always looks at
// a fixed target (the middle of the garden). Left/right change the yaw on the arc, up/down the
// height; both are clamped so the camera never leaves the front of the cabinet.
class OrbitCamera {
public:
    struct Limits {
        float minYawDeg = -40.0f, maxYawDeg = 40.0f; // 0 = straight in front of the cabinet
        float minHeight = 0.55f, maxHeight = 2.4f;   // world-space y of the camera
    };

    OrbitCamera(glm::vec3 target, float radius, Limits limits);

    // yawInput/heightInput in [-1, 1] (from the arrow keys), dt in seconds.
    void update(float yawInput, float heightInput, float dt);

    glm::vec3 position() const;
    glm::vec3 target() const { return target_; }
    glm::mat4 view() const;
    // Perspective projection. lensShiftY moves the image up in normalized device units, so the
    // target can be framed above a HUD panel while the camera still looks straight at it.
    glm::mat4 projection(float aspect, float lensShiftY = 0.0f) const;

    float yawDeg() const { return yawDeg_; }
    float height() const { return height_; }
    float fovDeg() const { return fovDeg_; }

private:
    glm::vec3 target_;
    float radius_;
    Limits limits_;
    float yawDeg_ = 0.0f;
    float height_;
    float fovDeg_ = 45.0f;
    float yawSpeedDeg_ = 45.0f; // degrees per second
    float heightSpeed_ = 1.0f;  // units per second
};
