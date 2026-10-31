#pragma once

#include <learnopengl/shader.h>

#include <glm/glm.hpp>

#include <map>
#include <string>

namespace engine {

// FreeType text rendering (LearnOpenGL "Text Rendering" chapter): every glyph is rasterized
// once into its own single-channel texture and drawn as a textured quad.
// Coordinates follow the projection passed to setProjection (origin bottom-left, y up);
// (x, y) is the baseline start. Input strings are UTF-8 (Latin-1 Supplement and Latin Extended-A
// are preloaded, so names like "Ćetković" render correctly).
class TextRenderer {
public:
    TextRenderer(const std::string& fontPath, unsigned int pixelHeight);
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    void setProjection(const glm::mat4& projection);
    void draw(const std::string& text, float x, float y, float scale, glm::vec4 color);

    float width(const std::string& text, float scale) const;
    float lineHeight(float scale) const { return float(pixelHeight_) * scale; }
    // Height of capital letters above the baseline, handy for vertical centering.
    float capHeight(float scale) const { return capHeight_ * scale; }

private:
    struct Glyph {
        unsigned int texture = 0;
        glm::ivec2 size{0};    // bitmap size in pixels
        glm::ivec2 bearing{0}; // offset from baseline to left/top of the glyph
        unsigned int advance = 0; // horizontal advance in 1/64 pixels
    };

    const Glyph* glyph(char32_t c) const;

    Shader shader_;
    std::map<char32_t, Glyph> glyphs_;
    unsigned int pixelHeight_;
    float capHeight_ = 0.0f;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
};

} // namespace engine
