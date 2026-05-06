#include "vertex_layout.h"
#include "diagnostics/gl_log.h"
#include <memory>

namespace SJH
{
    VertexLayoutUPtr VertexLayout::Create()
    {
        auto vao = std::unique_ptr<VertexLayout>(new VertexLayout());
        vao->Init();
        return std::move(vao);
    }

    VertexLayout::~VertexLayout()
    {
        if (mVertexArrayObject != 0)
            glDeleteVertexArrays(1, &mVertexArrayObject);
    }

    bool VertexLayout::Bind() const
    {
        glBindVertexArray(mVertexArrayObject);
        return Diagnostics::GLDebug::CheckGLBindVertexArray(mVertexArrayObject);
    }

    bool VertexLayout::TrySetAttrib(GLuint attrib_index, int count, GLuint type, bool normalized, GLsizei stride, uint64_t offset)
    {
        glEnableVertexAttribArray(attrib_index);
        if (!SJH::Diagnostics::GLDebug::CheckGLEnableVertexAttribArray(attrib_index))
            return false;

        glVertexAttribPointer(attrib_index, count, type, normalized, stride, (const void *)offset);
        if (!SJH::Diagnostics::GLDebug::CheckGLVertexAttribPointer({stride}))
            return false;

        return true;
    }

    void VertexLayout::DisableAttrib(int attrib_idx) const
    {
    }

    void VertexLayout::Init()
    {
        glGenVertexArrays(1, &mVertexArrayObject);
        Bind();
    }

}
