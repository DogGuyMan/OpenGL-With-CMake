#include "context.h"
#include "diagnostics/gl_log.h"
#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace SJH
{
    static std::array<float, 9> vertices = {
        -0.5f,
        -0.5f,
        0.0f,
        0.5f,
        -0.5f,
        0.0f,
        0.0f,
        0.5f,
        0.0f,
    };

    ContextUPtr Context::Create()
    {
        auto context = ContextUPtr(new Context());
        if (!context->Init())
            return nullptr;
        return std::move(context);
    }

    void Context::Render()
    {
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(mProgram->GetProgramAddr());
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    bool Context::Init()
    {
        // ShaderUPtr -> ShaderPtr 암묵 변환 — Program::Create 가 shared 입력을 요구
        SJH::ShaderPtr vertexShader = SJH::Shader::CreateFromFile("./resources/shader/simple.vs", GL_VERTEX_SHADER);
        SJH::ShaderPtr fragmentShader = SJH::Shader::CreateFromFile("./resources/shader/simple.fs", GL_FRAGMENT_SHADER);
        if (!vertexShader || !fragmentShader)
            return false;
        SPDLOG_INFO("vertex shader id: {}", vertexShader->GetShaderAddr());
        SPDLOG_INFO("fragment shader id: {}", fragmentShader->GetShaderAddr());

        mProgram = SJH::Program::Create({vertexShader, fragmentShader});
        if (!mProgram)
            return false;
        SPDLOG_INFO("program id: {}", mProgram->GetProgramAddr());
        glClearColor(0.0, 0.1f, 0.2f, 0.0f);

        /*
        순서
            1. VAO
            2. VBO
            3. Vertex Attribute Setting
        */

        // === VAO 생성 ===
        glGenVertexArrays(1, &mVertexArrayObject);
        if (!SJH::Diagnostics::GLDebug::CheckGLGenVertexArrays())
            return false;
        // === 지금부터 사용할 VAO 설정 ===
        glBindVertexArray(mVertexArrayObject);
        if (!SJH::Diagnostics::GLDebug::CheckGLBindVertexArray(mVertexArrayObject))
            return false;

        // === buffer object를 만든다 ===
        glGenBuffers(1, &mVertexBufferObject);
        if (!SJH::Diagnostics::GLDebug::CheckGLGenBuffers(mVertexBufferObject))
            return false;

        // === 지금부터 사용할 buffer object를 지정한다. ===
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferObject);
        if (!SJH::Diagnostics::GLDebug::CheckGLBindBuffer(mVertexBufferObject))
            return false;
        // === 사용할 buffer object는 vertex data를 저장할 용도 ===
        // === 변경빈도(STATIC=한번 / DYNAMIC=자주 / STREAM=매프레임) x 용도(DRAW=앱-> GPU / READ=GPU -> 앱 / COPY=GPU <-> GPU) ===
        const GLint dataSize = vertices.size() * sizeof(float);
        glBufferData(GL_ARRAY_BUFFER, dataSize, vertices.data(), GL_STATIC_DRAW);
        if (!SJH::Diagnostics::GLDebug::CheckGLBufferData(dataSize))
            return false;

        // === 셰이더 쪽 layout(location = 0) 인곳에 넣겠다. ===

        glEnableVertexAttribArray(0);
        if (!SJH::Diagnostics::GLDebug::CheckGLEnableVertexAttribArray(0))
            return false;

        void *positionOffset = 0;
        GLuint positionStride = sizeof(float) * 3;
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, positionStride, positionOffset);
        if (!SJH::Diagnostics::GLDebug::CheckGLVertexAttribPointer({positionStride}))
            return false;

        return true;
    }
}
