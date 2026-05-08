#version 330 core

in vec3 normalVector;
in vec4 vertexColor;
in vec2 texCoord;

// Rasterization 과정을 거쳐 픽셀별로 할당된 vertex shader의 출력값이 입력됨
out vec4 fragColor;

uniform vec4 baseColor;
uniform vec4 lightColor;
uniform float ambientStrength;
uniform sampler2D tex0;
uniform sampler2D tex1;

void main() {
    // vec4 ambient = ambientStrength * lightColor;
    vec4 tex0Color = texture(tex0, texCoord);
    vec4 tex1Color = texture(tex1, texCoord);

    float w = smoothstep(0.2, 0.8, texCoord.x);
    fragColor = mix(tex0Color, tex1Color, w);

    fragColor = baseColor * vertexColor * fragColor;
    // fragColor = ambient * baseColor * vertexColor;
}
