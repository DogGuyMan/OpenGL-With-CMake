#version 330 core
layout(location = 0) in vec3 positions;

void main() {
    gl_Position = vec4(positions, 1.0);
}
