#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix; // transpose(inverse(mat3(uModel))), computed on the CPU

void main()
{
    vec4 world = uModel * vec4(aPos, 1.0);
    FragPos = world.xyz;
    Normal = uNormalMatrix * aNormal;
    TexCoord = aTexCoord;
    gl_Position = uProjection * uView * world;
}
