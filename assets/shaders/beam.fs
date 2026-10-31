#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
out vec4 FragColor;

// Semi-transparent light beam under the lamp: brightest at the lamp, fading towards the floor,
// and softer where the surface is seen edge-on.
uniform vec3 uColor;
uniform float uAlpha;
uniform float uTopY;
uniform float uBottomY;
uniform vec3 uViewPos;

void main()
{
    float t = clamp((uTopY - FragPos.y) / (uTopY - uBottomY), 0.0, 1.0);
    vec3 n = normalize(Normal);
    float facing = abs(dot(n, normalize(uViewPos - FragPos)));
    float alpha = uAlpha * mix(1.0, 0.04, t) * (0.35 + 0.65 * facing);
    FragColor = vec4(uColor, alpha);
}
