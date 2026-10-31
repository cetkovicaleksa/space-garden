#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
out vec4 FragColor;

uniform vec3 uColor;

void main()
{
    // Placeholder shading until the lamp light is added: fixed directional light.
    vec3 n = normalize(Normal);
    float diff = max(dot(n, normalize(vec3(0.4, 1.0, 0.6))), 0.0);
    FragColor = vec4(uColor * (0.25 + 0.75 * diff), 1.0);
}
