#version 330 core

uniform vec4 baseColor;
uniform sampler2D tex;

in vec4 vertexColor;
in vec2 texCoord;

// Rasterization 과정을 거쳐 픽셀별로 할당된 vertex shader의 출력값이 입력됨
out vec4 fragColor;

void main() {
    fragColor = texture(tex, texCoord);
    fragColor = fragColor * vertexColor;
    fragColor = fragColor * baseColor;
}
