#version 330 core

in vec4 vertexColor; // vs에 이미 out으로 정의된 vertexColor가 입력될 변수임 ( 같은 변수명이며, 같은 타입이여야함 ) 
out vec4 FragColor; // 최종 출력 색상

void main() 
{
    FragColor = vertexColor;
}