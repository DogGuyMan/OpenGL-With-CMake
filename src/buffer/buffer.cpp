#include "buffer/buffer.h"
#include <memory>
#include "diagnostics/gl_log.h"

namespace SJH
{
    // EX
    /*
     * GL_ARRAY_BUFFER : GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertices, sizeof(float) * 12
     * GL_ELEMENT_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, indices, sizeof(uint32_t) * 6
     */
    BufferUPtr Buffer::CreateWithData(GLuint buffer_type, GLuint usage, const void *data, size_t data_size)
    {
        auto buffer = std::unique_ptr<Buffer>(new Buffer());
        if (!buffer->Init(buffer_type, usage, data, data_size))
            return nullptr;
        return std::move(buffer);
    }

    Buffer::~Buffer()
    {
        if (mBuffer != 0)
            glDeleteBuffers(1, &mBuffer);
    }

    bool Buffer::Bind() const
    {
        glBindBuffer(mBufferType, mBuffer);
        return Diagnostics::GLDebug::CheckGLBindBuffer(mBuffer);
    }

    bool Buffer::Init(GLuint buffer_type, GLuint usage, const void *data, size_t data_size)
    {
        mBufferType = buffer_type;
        mUsage = usage;
        glGenBuffers(1, &mBuffer);
        if (!Diagnostics::GLDebug::CheckGLGenBuffers(mBuffer))
            return false;
        if(!Bind()) {return false;}
        glBufferData(buffer_type, data_size, data, usage);
        if (!Diagnostics::GLDebug::CheckGLBufferData(data_size))
            return false;
        return true;
    }
}
