#pragma once

#include <string>

namespace engine {

// 2D texture loaded with stb_image (flipped on load so UV (0,0) is the bottom-left of the image).
class Texture2D {
public:
    Texture2D() = default;
    explicit Texture2D(const std::string& path, bool repeat = false, bool pixelated = false);
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& o) noexcept;
    Texture2D& operator=(Texture2D&& o) noexcept;

    bool load(const std::string& path, bool repeat = false, bool pixelated = false);
    void bind(unsigned int unit = 0) const;

    unsigned int id() const { return id_; }
    int width() const { return width_; }
    int height() const { return height_; }
    bool valid() const { return id_ != 0; }

private:
    void release();

    unsigned int id_ = 0;
    int width_ = 0;
    int height_ = 0;
};

} // namespace engine
