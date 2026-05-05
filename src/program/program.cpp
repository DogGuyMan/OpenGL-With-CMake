#include "program/program.h"
#include "diagnostics/gl_log.h"

namespace SJH
{
    ProgramUPtr Program::Create(const std::vector<ShaderPtr> &shaders)
    {
        auto program = ProgramUPtr(new Program());
        if (!program->TryLink(shaders))
            return nullptr;

        return std::move(program);
    }

    Program::~Program()
    {
        if (mProgramAddr != 0)
            glDeleteProgram(mProgramAddr);
    }

    bool Program::TryLink(const std::vector<ShaderPtr> &shaders)
    {
        mProgramAddr = glCreateProgram();
        for (auto &shader : shaders)
            glAttachShader(mProgramAddr, shader->GetShaderAddr());

        glLinkProgram(mProgramAddr);
        return SJH::diagnostics::GLObjectLog::CheckProgramLink(mProgramAddr);
    }
}
