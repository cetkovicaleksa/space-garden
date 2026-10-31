#include "GpuMesh.h"

#include <glad/glad.h>

#include <cstddef>
#include <utility>

GpuMesh::GpuMesh(const prim::MeshData& data)
    : indexCount_(static_cast<int>(data.indices.size()))
{
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(data.vertices.size() * sizeof(prim::Vertex)), data.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(data.indices.size() * sizeof(std::uint32_t)), data.indices.data(), GL_STATIC_DRAW);

    const GLsizei stride = sizeof(prim::Vertex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(prim::Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(prim::Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(prim::Vertex, uv)));
    glBindVertexArray(0);
}

GpuMesh::~GpuMesh() { release(); }

GpuMesh::GpuMesh(GpuMesh&& o) noexcept
    : vao_(std::exchange(o.vao_, 0u))
    , vbo_(std::exchange(o.vbo_, 0u))
    , ebo_(std::exchange(o.ebo_, 0u))
    , indexCount_(std::exchange(o.indexCount_, 0))
{
}

GpuMesh& GpuMesh::operator=(GpuMesh&& o) noexcept
{
    if (this != &o) {
        release();
        vao_ = std::exchange(o.vao_, 0u);
        vbo_ = std::exchange(o.vbo_, 0u);
        ebo_ = std::exchange(o.ebo_, 0u);
        indexCount_ = std::exchange(o.indexCount_, 0);
    }
    return *this;
}

void GpuMesh::release()
{
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        glDeleteBuffers(1, &vbo_);
        glDeleteBuffers(1, &ebo_);
    }
    vao_ = vbo_ = ebo_ = 0;
}

void GpuMesh::draw() const
{
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
