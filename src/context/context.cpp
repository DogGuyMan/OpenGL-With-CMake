#include "context.h"
#include <spdlog/spdlog.h>
#include <glad/glad.h>

namespace SJH
{
    ContextUPtr Context::Create()
    {
        auto context = ContextUPtr(new Context());
        if (!context->Init())
            return nullptr;
        return std::move(context);
    }

    void Context::Render()
    {
        glClear(GL_COLOR_BUFFER_BIT); // 프레임 버퍼 클리어

        // 딸랑 점 하나 그리기.
        glUseProgram(mProgram->GetProgramAddr());
        glPointSize(10.0f);
        glDrawArrays(GL_POINTS, 0, 1);
    }

    bool Context::Init()
    {
        // Shader 인스턴스가 unique_ptr에서 shared_ptr로 변환되었음을 유의
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
        glClearColor(0.0, 0.1f, 0.2f, 0.0f); // 프레임 버퍼에 씌울 컬러 지정

        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        return true;
    }
}
