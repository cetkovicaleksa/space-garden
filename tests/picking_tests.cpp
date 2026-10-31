// Unit tests for the 3D app's ray picking math (app3d/Picking.h), run with ctest.
#include "Picking.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <cstdio>

using namespace picking;

namespace {

int g_failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::printf("  FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
            ++g_failures;                                                        \
        }                                                                        \
    } while (0)

#define CHECK_NEAR(a, b, eps) CHECK(std::fabs((a) - (b)) < (eps))

void rayFromAboveHitsTopCap()
{
    const Ray down{{0.0f, 5.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
    const auto t = intersectCylinder(down, {0.0f, 0.0f, 0.0f}, 0.5f, 2.0f);
    CHECK(t.has_value());
    if (t) CHECK_NEAR(*t, 3.0f, 1e-4f);
}

void rayFromTheSideHitsTheWall()
{
    const Ray side{{0.0f, 1.0f, 5.0f}, {0.0f, 0.0f, -1.0f}};
    const auto t = intersectCylinder(side, {0.0f, 0.0f, 0.0f}, 0.5f, 2.0f);
    CHECK(t.has_value());
    if (t) CHECK_NEAR(*t, 4.5f, 1e-4f);
}

void raysThatMiss()
{
    // Passes beside, above, and points away from the cylinder.
    CHECK(!intersectCylinder({{1.0f, 1.0f, 5.0f}, {0.0f, 0.0f, -1.0f}}, {0, 0, 0}, 0.5f, 2.0f));
    CHECK(!intersectCylinder({{0.0f, 2.5f, 5.0f}, {0.0f, 0.0f, -1.0f}}, {0, 0, 0}, 0.5f, 2.0f));
    CHECK(!intersectCylinder({{0.0f, 1.0f, 5.0f}, {0.0f, 0.0f, 1.0f}}, {0, 0, 0}, 0.5f, 2.0f));
}

void cylinderBaseOffsetIsRespected()
{
    const Ray side{{3.0f, 0.7f, 5.0f}, {0.0f, 0.0f, -1.0f}};
    CHECK(intersectCylinder(side, {3.0f, 0.5f, -1.0f}, 0.25f, 0.4f).has_value());
    CHECK(!intersectCylinder(side, {3.0f, 0.8f, -1.0f}, 0.25f, 0.4f).has_value());
}

void screenRayPassesThroughProjectedPoint()
{
    const int w = 1600, h = 900;
    const glm::mat4 view = glm::lookAt(glm::vec3(1.5f, 1.4f, 3.2f), glm::vec3(-0.05f, 0.88f, -0.6f), glm::vec3(0, 1, 0));
    // Includes the lens shift used to frame the scene above the HUD panel.
    const glm::mat4 proj = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.25f, 0.0f)) *
                           glm::perspective(glm::radians(45.0f), float(w) / float(h), 0.05f, 50.0f);
    const glm::vec3 point(0.2f, 0.5f, -0.6f);
    glm::vec4 clip = proj * view * glm::vec4(point, 1.0f);
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    const double px = (ndc.x + 1.0) * 0.5 * w;
    const double py = (1.0 - ndc.y) * 0.5 * h;

    const Ray ray = rayFromScreen(px, py, w, h, view, proj);
    // Distance from the point to the ray.
    const glm::vec3 v = point - ray.origin;
    const float along = glm::dot(v, ray.dir);
    CHECK(along > 0.0f);
    CHECK_NEAR(glm::length(v - along * ray.dir), 0.0f, 1e-3f);
    CHECK_NEAR(glm::length(ray.dir), 1.0f, 1e-5f);
}

} // namespace

int main()
{
    struct { const char* name; void (*fn)(); } tests[] = {
        {"ray from above hits top cap", rayFromAboveHitsTopCap},
        {"ray from the side hits the wall", rayFromTheSideHitsTheWall},
        {"rays that miss", raysThatMiss},
        {"cylinder base offset is respected", cylinderBaseOffsetIsRespected},
        {"screen ray passes through projected point", screenRayPassesThroughProjectedPoint},
    };
    for (const auto& t : tests) {
        const int before = g_failures;
        t.fn();
        std::printf("%s %s\n", g_failures == before ? "[ OK ]" : "[FAIL]", t.name);
    }
    std::printf("%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
