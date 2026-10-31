#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uImage;
uniform int uMode;     // 0 = flat color, 1 = texture * tint, 2 = vertical gradient
uniform vec4 uColor;   // tint / flat color / gradient bottom
uniform vec4 uColor2;  // gradient top

void main()
{
    if (uMode == 1)
        FragColor = texture(uImage, TexCoord) * uColor;
    else if (uMode == 2)
        FragColor = mix(uColor, uColor2, TexCoord.y);
    else
        FragColor = uColor;
}
