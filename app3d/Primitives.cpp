#include "Primitives.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>

namespace prim {

void MeshData::addTriangle(const Vertex& a, const Vertex& b, const Vertex& c, glm::vec3 facing)
{
    const glm::vec3 n = glm::cross(b.position - a.position, c.position - a.position);
    if (glm::dot(n, n) < 1e-14f)
        return; // degenerate (e.g. at the pole of a surface of revolution)
    const auto base = static_cast<std::uint32_t>(vertices.size());
    vertices.push_back(a);
    if (glm::dot(n, facing) >= 0.0f) {
        vertices.push_back(b);
        vertices.push_back(c);
    } else {
        vertices.push_back(c);
        vertices.push_back(b);
    }
    indices.insert(indices.end(), {base, base + 1, base + 2});
}

void MeshData::addQuad(const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d, glm::vec3 facing)
{
    addTriangle(a, b, c, facing);
    addTriangle(a, c, d, facing);
}

void MeshData::append(const MeshData& other)
{
    const auto base = static_cast<std::uint32_t>(vertices.size());
    vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());
    for (std::uint32_t i : other.indices)
        indices.push_back(base + i);
}

namespace {

// Planar UV for a face: project onto the two axes that are most perpendicular to the normal.
glm::vec2 planarUv(glm::vec3 p, glm::vec3 n, float uvPerUnit)
{
    const glm::vec3 a = glm::abs(n);
    if (a.x >= a.y && a.x >= a.z)
        return glm::vec2(n.x > 0 ? -p.z : p.z, p.y) * uvPerUnit;
    if (a.y >= a.z)
        return glm::vec2(p.x, n.y > 0 ? -p.z : p.z) * uvPerUnit;
    return glm::vec2(n.z > 0 ? p.x : -p.x, p.y) * uvPerUnit;
}

void flatQuad(MeshData& m, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 centroid, float uvPerUnit)
{
    glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
    const glm::vec3 faceCenter = 0.25f * (a + b + c + d);
    if (glm::dot(n, faceCenter - centroid) < 0.0f)
        n = -n; // outward
    auto v = [&](glm::vec3 p) { return Vertex{p, n, planarUv(p, n, uvPerUnit)}; };
    m.addQuad(v(a), v(b), v(c), v(d), n);
}

} // namespace

MeshData hexahedron(const glm::vec3 (&c)[8], float uvPerUnit)
{
    glm::vec3 centroid(0.0f);
    for (const glm::vec3& p : c)
        centroid += p * 0.125f;
    MeshData m;
    flatQuad(m, c[0], c[1], c[2], c[3], centroid, uvPerUnit); // bottom
    flatQuad(m, c[4], c[5], c[6], c[7], centroid, uvPerUnit); // top
    for (int i = 0; i < 4; ++i) {
        const int j = (i + 1) % 4;
        flatQuad(m, c[i], c[j], c[j + 4], c[i + 4], centroid, uvPerUnit);
    }
    return m;
}

MeshData box(glm::vec3 lo, glm::vec3 hi, float uvPerUnit)
{
    const glm::vec3 corners[8] = {
        {lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z}, {hi.x, lo.y, hi.z}, {lo.x, lo.y, hi.z},
        {lo.x, hi.y, lo.z}, {hi.x, hi.y, lo.z}, {hi.x, hi.y, hi.z}, {lo.x, hi.y, hi.z},
    };
    return hexahedron(corners, uvPerUnit);
}

MeshData lathe(const std::vector<ProfileSegment>& profile, int slices, bool smoothCurve)
{
    MeshData m;
    // Total profile length for the v coordinate.
    float total = 0.0f;
    for (const ProfileSegment& s : profile)
        total += glm::length(s.b - s.a);
    if (total <= 0.0f)
        total = 1.0f;

    // Walking a segment a -> b, the outside of the solid is on the right-hand side, so the outward
    // normal is the direction rotated clockwise. Each segment has its own normal (sharp edges)
    // unless smoothCurve averages it with the neighbours.
    float v0 = 0.0f;
    for (std::size_t si = 0; si < profile.size(); ++si) {
        const ProfileSegment& s = profile[si];
        const glm::vec2 d = s.b - s.a;
        const float len = glm::length(d);
        if (len <= 0.0f)
            continue;
        glm::vec2 n2 = glm::normalize(glm::vec2(d.y, -d.x));
        glm::vec2 na = n2, nb = n2;
        if (smoothCurve) {
            // Average with neighbours for curved profiles (spheres, drops).
            auto segNormal = [&](std::size_t k) {
                const glm::vec2 dk = profile[k].b - profile[k].a;
                return glm::length(dk) > 0 ? glm::normalize(glm::vec2(dk.y, -dk.x)) : n2;
            };
            if (si > 0) na = glm::normalize(n2 + segNormal(si - 1));
            if (si + 1 < profile.size()) nb = glm::normalize(n2 + segNormal(si + 1));
        }
        const float v1 = v0 + len / total;
        for (int i = 0; i < slices; ++i) {
            const float t0 = float(i) / float(slices);
            const float t1 = float(i + 1) / float(slices);
            const float a0 = t0 * glm::two_pi<float>();
            const float a1 = t1 * glm::two_pi<float>();
            auto vert = [&](glm::vec2 rp, glm::vec2 n, float angle, float u, float v) {
                const float cs = std::cos(angle), sn = std::sin(angle);
                return Vertex{{rp.x * cs, rp.y, rp.x * sn}, glm::normalize(glm::vec3(n.x * cs, n.y, n.x * sn)), {u, v}};
            };
            const Vertex p0 = vert(s.a, na, a0, t0, v0);
            const Vertex p1 = vert(s.a, na, a1, t1, v0);
            const Vertex p2 = vert(s.b, nb, a1, t1, v1);
            const Vertex p3 = vert(s.b, nb, a0, t0, v1);
            const float am = 0.5f * (a0 + a1);
            const glm::vec3 facing(n2.x * std::cos(am), n2.y, n2.x * std::sin(am));
            m.addQuad(p0, p1, p2, p3, facing);
        }
        v0 = v1;
    }
    return m;
}

// Profiles below are walked so that the outside of the solid is on the right-hand side
// (clockwise around the solid's cross-section in the (r, y) plane).

MeshData frustum(float rb, float rt, float h, int slices)
{
    return lathe({
                     {{0.0f, 0.0f}, {rb, 0.0f}}, // bottom cap, normal -y
                     {{rb, 0.0f}, {rt, h}},      // side
                     {{rt, h}, {0.0f, h}},       // top cap, normal +y
                 },
                 slices);
}

MeshData cylinder(float radius, float height, int slices) { return frustum(radius, radius, height, slices); }

float PotDims::innerRadiusAt(float y) const
{
    const float t = std::clamp(y / height, 0.0f, 1.0f);
    return bottomRadius + (topRadius - bottomRadius) * t - wall;
}

MeshData potShell(const PotDims& p, int slices)
{
    const float hs = p.soilY();
    return lathe({
                     {{0.0f, 0.0f}, {p.bottomRadius, 0.0f}},                         // bottom
                     {{p.bottomRadius, 0.0f}, {p.topRadius, p.height}},              // outer wall
                     {{p.topRadius, p.height}, {p.topRadius - p.wall, p.height}},    // rim
                     {{p.topRadius - p.wall, p.height}, {p.innerRadiusAt(hs), hs}},  // inner wall
                 },
                 slices);
}

MeshData potSoil(const PotDims& p, int slices)
{
    const float hs = p.soilY();
    return lathe({{{p.innerRadiusAt(hs), hs}, {0.0f, hs}}}, slices); // disc, normal +y
}

MeshData ring(float ri, float ro, float t, int slices)
{
    return lathe({
                     {{ri, 0.0f}, {ro, 0.0f}}, // bottom
                     {{ro, 0.0f}, {ro, t}},    // outer
                     {{ro, t}, {ri, t}},       // top
                     {{ri, t}, {ri, 0.0f}},    // inner
                 },
                 slices);
}

MeshData sphere(float radius, int slices, int stacks)
{
    std::vector<ProfileSegment> profile;
    for (int i = 0; i < stacks; ++i) {
        // From the south pole up to the north pole along the outside (clockwise in (r, y)).
        const float a0 = -glm::half_pi<float>() + glm::pi<float>() * float(i) / float(stacks);
        const float a1 = -glm::half_pi<float>() + glm::pi<float>() * float(i + 1) / float(stacks);
        profile.push_back({{radius * std::cos(a0), radius * std::sin(a0)}, {radius * std::cos(a1), radius * std::sin(a1)}});
    }
    MeshData m = lathe(profile, slices, true);
    for (Vertex& v : m.vertices)
        v.normal = glm::normalize(v.position); // exact normals
    return m;
}

} // namespace prim
