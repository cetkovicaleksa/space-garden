#include "engine/Texture.h"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <utility>

namespace engine {

Texture2D::Texture2D(const std::string& path, bool repeat, bool pixelated) { load(path, repeat, pixelated); }

Texture2D::~Texture2D() { release(); }

Texture2D::Texture2D(Texture2D&& o) noexcept
    : id_(std::exchange(o.id_, 0)), width_(o.width_), height_(o.height_) {}

Texture2D& Texture2D::operator=(Texture2D&& o) noexcept
{
    if (this != &o) {
        release();
        id_ = std::exchange(o.id_, 0);
        width_ = o.width_;
        height_ = o.height_;
    }
    return *this;
}

void Texture2D::release()
{
    if (id_)
        glDeleteTextures(1, &id_);
    id_ = 0;
}

bool Texture2D::load(const std::string& path, bool repeat, bool pixelated)
{
    release();
    stbi_set_flip_vertically_on_load(true);
    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width_, &height_, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << '\n';
        return false;
    }

    GLenum format = GL_RGBA;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;

    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GLint(format), width_, height_, 0, format, GL_UNSIGNED_BYTE, data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glGenerateMipmap(GL_TEXTURE_2D);

    const GLint wrap = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, pixelated ? GL_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, pixelated ? GL_NEAREST : GL_LINEAR);
    if (channels == 1) {
        const GLint swizzle[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    return true;
}

void Texture2D::bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}

} // namespace engine
