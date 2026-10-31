#include "engine/TextRenderer.h"

#include "engine/Paths.h"

#include <glad/glad.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdexcept>
#include <vector>

namespace engine {

namespace {

// Minimal UTF-8 decoder; invalid bytes are returned as '?'.
std::vector<char32_t> decodeUtf8(const std::string& s)
{
    std::vector<char32_t> out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size();) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        char32_t cp = '?';
        size_t len = 1;
        if (c < 0x80) {
            cp = c;
        } else if ((c >> 5) == 0x6 && i + 1 < s.size()) {
            cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(s[i + 1]) & 0x3F);
            len = 2;
        } else if ((c >> 4) == 0xE && i + 2 < s.size()) {
            cp = ((c & 0x0F) << 12) | ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 6) |
                 (static_cast<unsigned char>(s[i + 2]) & 0x3F);
            len = 3;
        } else if ((c >> 3) == 0x1E && i + 3 < s.size()) {
            cp = ((c & 0x07) << 18) | ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 12) |
                 ((static_cast<unsigned char>(s[i + 2]) & 0x3F) << 6) | (static_cast<unsigned char>(s[i + 3]) & 0x3F);
            len = 4;
        }
        out.push_back(cp);
        i += len;
    }
    return out;
}

} // namespace

TextRenderer::TextRenderer(const std::string& fontPath, unsigned int pixelHeight)
    : shader_(shaderPath("text.vs"), shaderPath("text.fs"))
    , pixelHeight_(pixelHeight)
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
        throw std::runtime_error("Could not initialize FreeType");
    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        FT_Done_FreeType(ft);
        throw std::runtime_error("Failed to load font: " + fontPath);
    }
    FT_Set_Pixel_Sizes(face, 0, pixelHeight);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // glyph bitmaps are tightly packed, 1 byte per pixel

    auto loadRange = [&](char32_t first, char32_t last) {
        for (char32_t c = first; c <= last; ++c) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
                continue;
            Glyph g;
            glGenTextures(1, &g.texture);
            glBindTexture(GL_TEXTURE_2D, g.texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, int(face->glyph->bitmap.width), int(face->glyph->bitmap.rows), 0,
                         GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            g.size = {int(face->glyph->bitmap.width), int(face->glyph->bitmap.rows)};
            g.bearing = {face->glyph->bitmap_left, face->glyph->bitmap_top};
            g.advance = static_cast<unsigned int>(face->glyph->advance.x);
            glyphs_[c] = g;
        }
    };
    loadRange(32, 126);       // ASCII
    loadRange(0xA0, 0x17F);   // Latin-1 Supplement + Latin Extended-A (č ć š ž đ ...)
    loadRange(0x2022, 0x2022); // bullet

    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    if (auto it = glyphs_.find('H'); it != glyphs_.end())
        capHeight_ = float(it->second.bearing.y);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // One dynamic quad (6 vertices of vec4: pos.xy, uv.xy), refilled per glyph.
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glBindVertexArray(0);

    shader_.use();
    shader_.setInt("uText", 0);
}

TextRenderer::~TextRenderer()
{
    for (auto& [c, g] : glyphs_)
        glDeleteTextures(1, &g.texture);
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void TextRenderer::setProjection(const glm::mat4& projection)
{
    shader_.use();
    shader_.setMat4("uProjection", projection);
}

const TextRenderer::Glyph* TextRenderer::glyph(char32_t c) const
{
    auto it = glyphs_.find(c);
    if (it == glyphs_.end())
        it = glyphs_.find('?');
    return it == glyphs_.end() ? nullptr : &it->second;
}

void TextRenderer::draw(const std::string& text, float x, float y, float scale, glm::vec4 color)
{
    shader_.use();
    shader_.setVec4("uColor", color);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao_);

    for (char32_t c : decodeUtf8(text)) {
        const Glyph* g = glyph(c);
        if (!g)
            continue;
        const float xpos = x + float(g->bearing.x) * scale;
        const float ypos = y - float(g->size.y - g->bearing.y) * scale;
        const float w = float(g->size.x) * scale;
        const float h = float(g->size.y) * scale;
        // FreeType bitmaps are stored top row first, so v = 0 is the top of the glyph.
        const float vertices[6][4] = {
            {xpos, ypos + h, 0.0f, 0.0f},
            {xpos, ypos, 0.0f, 1.0f},
            {xpos + w, ypos, 1.0f, 1.0f},
            {xpos, ypos + h, 0.0f, 0.0f},
            {xpos + w, ypos, 1.0f, 1.0f},
            {xpos + w, ypos + h, 1.0f, 0.0f},
        };
        glBindTexture(GL_TEXTURE_2D, g->texture);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += float(g->advance >> 6) * scale; // advance is in 1/64 pixels
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

float TextRenderer::width(const std::string& text, float scale) const
{
    float w = 0.0f;
    for (char32_t c : decodeUtf8(text))
        if (const Glyph* g = glyph(c))
            w += float(g->advance >> 6) * scale;
    return w;
}

} // namespace engine
