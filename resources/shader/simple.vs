#version 330 core
// 각 정점 별로 설정된 vertex attribute를 입력받는다
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;
layout(location = 3) in vec2 aTexCoord;

out vec3 normalVector;
out vec4 vertexColor;
out vec2 texCoord;
out vec3 positionVector;

uniform mat4 transformMat;
uniform mat4 modelTransformMat;

void main() {
    gl_Position = transformMat * vec4(aPos.xyz, 1.0);
    // 변환전 순수 local 좌표계에서의 normal을 전달해야함.
    normalVector = (transpose(inverse(modelTransformMat)) * vec4(aNormal, 0.0)).xyz;
    vertexColor = vec4(aColor.rgb, 1.0);
    texCoord = aTexCoord;
    // diffuse 값을 계산하기 위해 world space 상에서의 좌표값이 필요
    positionVector = (modelTransformMat * vec4(aPos, 1.0)).xyz;
}
