#version 330 core
// 각 정점 별로 설정된 vertex attribute를 입력받는다
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;
layout(location = 3) in vec2 aTexCoord;

out vec3 normalVector;
out vec4 vertexColor;
out vec2 texCoord;

uniform mat4 transformMat;

void main() {
    gl_Position = transformMat * vec4(aPos.xyz, 1.0);
    normalVector = aNormal;
    vertexColor = vec4(aColor.rgb, 1.0);
    texCoord = aTexCoord;
}
