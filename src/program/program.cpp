#include "program/program.h"
#include "diagnostics/gl_log.h"

namespace SJH
{
    ProgramUPtr Program::Create(const std::vector<ShaderPtr> &shaders)
    {
        auto program = ProgramUPtr(new Program());
        if (!program->TryLink(shaders))
            return nullptr;

        // link 성공 직후 uniform 캐시 eager build — 호출자는 즉시 Uniforms::Set* 호출 가능.
        Uniforms::BuildCache(*program);
        return program;
    }

    Program::~Program()
    {
        if (mProgramAddr != 0)
            glDeleteProgram(mProgramAddr);
    }

    void Program::Use() const {
        glUseProgram(mProgramAddr);
    }

    bool Program::TryLink(const std::vector<ShaderPtr> &shaders)
    {
        mProgramAddr = glCreateProgram();
        // 모든 셰이더를 program 에 attach — 링크 시 셰이더 단계가 결합됨
        for (auto &shader : shaders)
            glAttachShader(mProgramAddr, shader->GetShaderAddr());

        glLinkProgram(mProgramAddr);
        return SJH::Diagnostics::GLObjectLog::CheckProgramLink(mProgramAddr);
    }
}
