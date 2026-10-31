#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

// Procedural meshes with positions, normals and UVs. Every generator produces closed geometry
// with counter-clockwise front faces seen from outside, so back-face culling never removes a
// surface that could be visible (with depth testing on, culling does not change the picture).
namespace prim {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    // Triangle wound counter-clockwise around `facing` (vertex order is swapped if needed).
    void addTriangle(const Vertex& a, const Vertex& b, const Vertex& c, glm::vec3 facing);
    // Quad a-b-c-d (around its perimeter), wound counter-clockwise around `facing`.
    void addQuad(const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d, glm::vec3 facing);
    void append(const MeshData& other);
};

// Axis-aligned box. UVs are world size * uvPerUnit, so tiled textures keep their scale.
MeshData box(glm::vec3 min, glm::vec3 max, float uvPerUnit = 1.0f);

// Convex hexahedron from 8 corners: 0-3 bottom ring, 4-7 top ring (same order). Flat normals.
MeshData hexahedron(const glm::vec3 (&corners)[8], float uvPerUnit = 1.0f);

// Surface of revolution around the y axis. The profile is a list of segments in the (r, y)
// half-plane; each segment has its own outward normal, which keeps edges between segments sharp.
struct ProfileSegment {
    glm::vec2 a, b; // (radius, y)
};
MeshData lathe(const std::vector<ProfileSegment>& profile, int slices, bool smoothCurve = false);

// Closed cylinder (with caps) standing on y = 0.
MeshData cylinder(float radius, float height, int slices = 32);
// Closed truncated cone standing on y = 0.
MeshData frustum(float bottomRadius, float topRadius, float height, int slices = 32);
// Hollow pot standing on y = 0: outer wall, rim, inner wall down to the soil level and the bottom.
// The soil disc itself is returned separately so it can use its own material.
struct PotDims {
    float bottomRadius = 0.15f;
    float topRadius = 0.20f;
    float height = 0.34f;
    float wall = 0.025f;
    float soilDepth = 0.06f; // distance from the rim down to the soil surface
    float soilY() const { return height - soilDepth; }
    float innerRadiusAt(float y) const;
};
MeshData potShell(const PotDims& dims, int slices = 48);
MeshData potSoil(const PotDims& dims, int slices = 48);
// Flat ring (closed, with thickness) around the y axis, used for the selection highlight.
MeshData ring(float innerRadius, float outerRadius, float thickness, int slices = 64);
// UV sphere centered at the origin.
MeshData sphere(float radius, int slices = 24, int stacks = 16);

} // namespace prim
