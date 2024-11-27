#include "vertex_layout.h"

VertexLayoutUPtr VertexLayout::Create() {
    VertexLayoutUPtr vertexLayout = VertexLayoutUPtr(new VertexLayout());
    vertexLayout->Init();
    return std::move(vertexLayout);
}

VertexLayout::~VertexLayout() {
    if(mVertexArrayObject) {
        glDeleteVertexArrays(DEFAULT_VAO_COUNT, &mVertexArrayObject);
    }
}

void VertexLayout::Bind() const {
    glBindVertexArray(mVertexArrayObject);
}

void VertexLayout::SetAttrib(
    uint32_t attribIndex, int count,
    uint32_t type, bool normalized,
    size_t stride, uint64_t offset) const 
{
    glEnableVertexAttribArray(attribIndex); 
    glVertexAttribPointer(
        attribIndex, count, 
        type, normalized, 
        stride, (const void*) offset
    );

}
void VertexLayout::DisableAttrib(int attribIndex) const {
    glDisableVertexAttribArray(attribIndex);
}

void VertexLayout::Init() {
    glGenVertexArrays(DEFAULT_VAO_COUNT, &mVertexArrayObject);
    Bind();
}