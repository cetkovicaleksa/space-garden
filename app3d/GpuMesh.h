#pragma once

#include "Primitives.h"

// Mesh uploaded to the GPU: VAO with position (0), normal (1) and uv (2), drawn with an EBO.
class GpuMesh {
public:
    GpuMesh() = default;
    explicit GpuMesh(const prim::MeshData& data);
    ~GpuMesh();

    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;
    GpuMesh(GpuMesh&& o) noexcept;
    GpuMesh& operator=(GpuMesh&& o) noexcept;

    void draw() const;
    bool valid() const { return vao_ != 0; }

private:
    void release();

    unsigned int vao_ = 0, vbo_ = 0, ebo_ = 0;
    int indexCount_ = 0;
};
