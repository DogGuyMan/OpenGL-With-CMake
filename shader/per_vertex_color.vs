#version 330 core
layout (location = 0) in vec3 aPos; // 0번째 attribute가 정점의 위치 attribute가 들어옴 '0' 이라는 이름을 가진 attribute다
layout (location = 1) in vec3 aColor; // 1번째 attribute가 정점의 위치 attribute가 들어옴 '1' 이라는 이름을 가진 attribute다

out vec4 vertexColor; // fragment shader로 넘어갈 컬러값

void main() {
    gl_Position = vec4(aPos, 1.0); // vec3를 vec4 생성자에 사용
    vertexColor = vec4(aColor, 1.0);
} 
