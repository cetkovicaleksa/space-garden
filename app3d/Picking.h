#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <optional>

// Mouse picking by ray casting: the cursor is turned into a world-space ray with the inverse of
// projection * view and tested against simple bounding volumes (pure math, no OpenGL).
namespace picking {

struct Ray {
    glm::vec3 origin;
    glm::vec3 dir; // normalized
};

// Ray through a framebuffer pixel (origin top-left) for the given camera matrices.
inline Ray rayFromScreen(double px, double py, int width, int height, const glm::mat4& view, const glm::mat4& projection)
{
    const float x = 2.0f * float(px) / float(width) - 1.0f;
    const float y = 1.0f - 2.0f * float(py) / float(height);
    const glm::mat4 inv = glm::inverse(projection * view);
    glm::vec4 nearP = inv * glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 farP = inv * glm::vec4(x, y, 1.0f, 1.0f);
    nearP /= nearP.w;
    farP /= farP.w;
    return {glm::vec3(nearP), glm::normalize(glm::vec3(farP - nearP))};
}

// Distance along the ray to a vertical capped cylinder standing on `base`, if it is hit.
inline std::optional<float> intersectCylinder(const Ray& ray, glm::vec3 base, float radius, float height)
{
    const glm::vec3 o = ray.origin - base;
    const glm::vec3 d = ray.dir;
    std::optional<float> best;
    auto consider = [&](float t) {
        if (t >= 0.0f && (!best || t < *best))
            best = t;
    };

    // Side: (ox + t dx)^2 + (oz + t dz)^2 = r^2 with 0 <= y <= height.
    const float a = d.x * d.x + d.z * d.z;
    if (a > 1e-8f) {
        const float b = 2.0f * (o.x * d.x + o.z * d.z);
        const float c = o.x * o.x + o.z * o.z - radius * radius;
        const float disc = b * b - 4.0f * a * c;
        if (disc >= 0.0f) {
            const float s = std::sqrt(disc);
            for (float t : {(-b - s) / (2.0f * a), (-b + s) / (2.0f * a)}) {
                const float y = o.y + t * d.y;
                if (y >= 0.0f && y <= height)
                    consider(t);
            }
        }
    }
    // Caps: planes y = 0 and y = height, inside the radius.
    if (std::abs(d.y) > 1e-8f) {
        for (float capY : {0.0f, height}) {
            const float t = (capY - o.y) / d.y;
            const float x = o.x + t * d.x, z = o.z + t * d.z;
            if (x * x + z * z <= radius * radius)
                consider(t);
        }
    }
    return best;
}

} // namespace picking
