#version 330 core

in vec3 normalVector;
in vec3 positionVector;
in vec4 vertexColor;
in vec2 texCoord;

// Rasterization 과정을 거쳐 픽셀별로 할당된 vertex shader의 출력값이 입력됨
out vec4 fragColor;

uniform vec3 lightPos;
uniform vec4 baseColor;
uniform vec3 lightColor;
uniform float ambientStrength;
uniform sampler2D tex0;
uniform sampler2D tex1;

void main() {
    vec4 ambient = ambientStrength * vec4(lightColor.rgb, 1.0f);
    vec4 tex0Color = texture(tex0, texCoord);
    vec4 tex1Color = texture(tex1, texCoord);

    float w = smoothstep(0.2, 0.8, texCoord.x);
    fragColor = baseColor * vertexColor;
    fragColor = mix(tex0Color, tex1Color, w) * fragColor;

    vec3 lightDir = normalize(lightPos - positionVector);
    // vertex shader에서 계산된 normal은 rasterization 되는 과정에서 선형 보간이 진행되서
    // FS에 와서도 normalize를 해줘야 함.
    vec3 pixelNorm = normalize(normalVector);
    vec3 diffuse = max(dot(pixelNorm, lightDir), 0.0) * lightColor;

    fragColor = (ambient + vec4(diffuse, 1.0)) * fragColor;
}
