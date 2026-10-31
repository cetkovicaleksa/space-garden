#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uText; // single channel glyph bitmap
uniform vec4 uColor;

void main()
{
    // The red channel holds glyph coverage and is used as alpha.
    FragColor = vec4(uColor.rgb, uColor.a * texture(uText, TexCoord).r);
}
