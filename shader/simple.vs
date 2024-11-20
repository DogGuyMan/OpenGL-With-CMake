#version 330 core
layout (location = 0) in vec3 aPos; //glVertexAttribPointer 의 0번 attribute가 들어옴

void main() {
    gl_Position = vec4(aPos, 1.0);
} 