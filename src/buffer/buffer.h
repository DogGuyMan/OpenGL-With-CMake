/**
 * @file buffer.h
 * @brief VBO/EBO RAII 래퍼 — @c glGenBuffers ~ @c glDeleteBuffers 자원 수명 관리.
 *
 * @details
 *  ### 개념
 *  - **VBO** (Vertex Buffer Object, @c GL_ARRAY_BUFFER) — 정점 *데이터*.
 *    CPU 메모리에 있는 정점 배열을 GPU 로 옮긴 raw byte 묶음.
 *    담기는 내용은 position, normal, tangent, color, texture, uv등등...
 *  - **EBO** (Element Buffer Object, @c GL_ELEMENT_ARRAY_BUFFER) — 인덱스 데이터.
 *    어떤 정점을 어떤 순서로 그릴지 지정.
 *  - 둘은 GL 객체 종류가 같고 (`glGenBuffers` 로 생성) @c bufferType 만 다름 -> 동일 클래스로 통합.
 *
 *  ### 다른 GL 객체와의 경계 — VAO는 별도
 *  - VBO가 가진 정점에 대한 구조(layout)를 알려줄 방법
 *  - **VAO** (Vertex Array Object, @c glGenVertexArrays) — 정점 데이터의 *구조* 를 알려주는 descriptor.
 *    예: Position 이 vec3/vec4 인지, Color 가 RGB(vec3)/RGBA(vec4) 인지, UV 가 vec2 인지,
 *    각각의 stride/offset 등.
 *  - VAO 책임은 이 클래스가 *아님* — @c SJH::VertexLayout 이 담당 (@c src/layout/).
 */

namespace SJH
{

}
