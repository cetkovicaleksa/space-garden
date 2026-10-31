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

// The garden lamp: a spotlight under the cabinet roof pointing down at the pots.
struct SpotLight {
    bool on;
    vec3 position;
    vec3 direction;
    float cutOff;      // cosine of the inner cone angle (full intensity inside)
    float outerCutOff; // cosine of the outer cone angle (no light outside)
    vec3 color;
    float constant;    // attenuation 1 / (c + l*d + q*d^2)
    float linear;
    float quadratic;
};

uniform Material uMaterial;
uniform bool uUseTexture;
uniform sampler2D uTexture;
uniform SpotLight uLight;
uniform vec3 uAmbient; // weak background light, keeps the scene visible with the lamp off
uniform vec3 uViewPos;

void main()
{
    vec4 texel = uUseTexture ? texture(uTexture, TexCoord) : vec4(1.0);
    vec3 base = uMaterial.color * texel.rgb;

    vec3 n = normalize(Normal);
    if (!gl_FrontFacing)
        n = -n; // only matters with culling off; such faces are hidden by the depth test anyway
    vec3 viewDir = normalize(uViewPos - FragPos);

    vec3 color = uAmbient * base;

    if (uLight.on) {
        vec3 toLight = uLight.position - FragPos;
        float dist = length(toLight);
        vec3 lightDir = toLight / dist;

        // Soft spotlight edge between the inner and outer cone.
        float theta = dot(lightDir, normalize(-uLight.direction));
        float spot = clamp((theta - uLight.outerCutOff) / (uLight.cutOff - uLight.outerCutOff), 0.0, 1.0);
        float attenuation = 1.0 / (uLight.constant + uLight.linear * dist + uLight.quadratic * dist * dist);

        // Blinn-Phong.
        float diff = max(dot(n, lightDir), 0.0);
        vec3 halfway = normalize(lightDir + viewDir);
        float spec = diff > 0.0 ? pow(max(dot(n, halfway), 0.0), uMaterial.shininess) : 0.0;

        color += (diff * base + spec * uMaterial.specular) * uLight.color * spot * attenuation;
    }

    color += uMaterial.emission * texel.rgb;
    FragColor = vec4(color, uMaterial.alpha * texel.a);
}
