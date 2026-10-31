#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 uModel;
uniform mat4 uProjection;
uniform vec2 uUvScale; // > 1 repeats the texture (tiling)

void main()
{
    TexCoord = aTexCoord * uUvScale;
    gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
}
