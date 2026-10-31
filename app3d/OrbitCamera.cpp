#include "OrbitCamera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

OrbitCamera::OrbitCamera(glm::vec3 target, float radius, Limits limits)
    : target_(target)
    , radius_(radius)
    , limits_(limits)
    , height_(std::clamp(target.y + 0.45f, limits.minHeight, limits.maxHeight))
{
}

void OrbitCamera::update(float yawInput, float heightInput, float dt)
{
    yawDeg_ = std::clamp(yawDeg_ + yawInput * yawSpeedDeg_ * dt, limits_.minYawDeg, limits_.maxYawDeg);
    height_ = std::clamp(height_ + heightInput * heightSpeed_ * dt, limits_.minHeight, limits_.maxHeight);
}

glm::vec3 OrbitCamera::position() const
{
    // Arc around the vertical axis through the target, in front of the cabinet (+z is the open side).
    const float yaw = glm::radians(yawDeg_);
    return {target_.x + radius_ * std::sin(yaw), height_, target_.z + radius_ * std::cos(yaw)};
}

glm::mat4 OrbitCamera::view() const
{
    return glm::lookAt(position(), target_, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 OrbitCamera::projection(float aspect, float lensShiftY) const
{
    const glm::mat4 shift = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, lensShiftY, 0.0f));
    return shift * glm::perspective(glm::radians(fovDeg_), aspect, 0.05f, 50.0f);
}
