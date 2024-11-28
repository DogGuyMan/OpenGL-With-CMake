#version 330 core

uniform vec4 color; // uniform 코드는 외부 cpp에서 받아올 예정이다.
out vec4 FragColor; 

void main() 
{
    FragColor = color;
}