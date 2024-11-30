#version 330 core

in vec4 vertexColor; // per_vertex_color.vs에서 가져올 인풋
out vec4 fragColor; 

void main() 
{
    fragColor = vertexColor;
}