#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
out vec4 FragColor;

struct Material {
    vec3 color;     // diffuse color (multiplies the texture when one is bound)
    float specular; // specular strength
    float shininess;
    vec3 emission;  // light emitted by the surface itself (lamp strip, space window)
    float alpha;
};

uniform Material uMaterial;
uniform bool uUseTexture;
uniform sampler2D uTexture;
uniform vec3 uViewPos;

void main()
{
    vec3 base = uMaterial.color;
    if (uUseTexture)
        base *= texture(uTexture, TexCoord).rgb;
    vec3 emission = uMaterial.emission;
    if (uUseTexture)
        emission *= texture(uTexture, TexCoord).rgb;

    // Placeholder shading until the lamp light is added: fixed directional light.
    vec3 n = normalize(Normal);
    float diff = max(dot(n, normalize(vec3(0.4, 1.0, 0.6))), 0.0);
    FragColor = vec4(base * (0.3 + 0.7 * diff) + emission, uMaterial.alpha);
}
