/**
 * @file program_uniforms.cpp
 * @brief @c ProgramUniforms 구현 — eager 캐시 빌드 + lazy 보강 + 진단 위임 (static).
 */

#include "program_uniforms.h"
#include "diagnostics/uniform_diagnostics.h"

#include <vector>

namespace SJH
{
    ProgramUniforms::ProgramUniforms(GLuint program) : mProgram(program)
    {
        // Eager build — 모든 active uniform 의 (name -> location, type) 을 미리 캐싱.
        // 비활성/배열원소(arr[3]) 등은 lazy 로 Lookup 시점에 보강.
        GLint count = 0;
        glGetProgramiv(mProgram, GL_ACTIVE_UNIFORMS, &count);
        if (count <= 0)
            return;

        GLint maxNameLen = 0;
        glGetProgramiv(mProgram, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLen);
        if (maxNameLen <= 0)
            return;

        std::vector<char> nameBuf(static_cast<size_t>(maxNameLen + 1), '\0');
        for (GLint i = 0; i < count; ++i)
        {
            GLsizei nameSize = 0;
            GLint   size     = 0;
            GLenum  type     = 0;
            glGetActiveUniform(mProgram, static_cast<GLuint>(i),
                               maxNameLen, &nameSize, &size, &type, nameBuf.data());

            const GLint loc = glGetUniformLocation(mProgram, nameBuf.data());
            mEntries.emplace(std::string(nameBuf.data(), static_cast<size_t>(nameSize)),
                             Entry{loc, type});
        }
    }

    const ProgramUniforms::Entry &ProgramUniforms::Lookup(const char *name)
    {
        const auto it = mEntries.find(name);
        if (it != mEntries.end())
            return it->second;

        // Cache miss — 배열 원소 같은 비-canonical 이름일 수 있음. lazy 보강.
        // 배열 원소 type 은 active 인덱스 탐색 필요 — 진단 가치 < 비용이라 type 은 0 유지.
        Entry e;
        e.location = glGetUniformLocation(mProgram, name);
        return mEntries.emplace(name, e).first->second;
    }

    void ProgramUniforms::SetMat4(const char *name, const float *mat4)
    {
        const auto &e = Lookup(name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(mProgram, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(mProgram, name, GL_FLOAT_MAT4, e.type);
        glUniformMatrix4fv(e.location, 1, GL_FALSE, mat4);
    }

    void ProgramUniforms::SetVec4(const char *name, const float *v4)
    {
        const auto &e = Lookup(name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(mProgram, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(mProgram, name, GL_FLOAT_VEC4, e.type);
        glUniform4fv(e.location, 1, v4);
    }

    void ProgramUniforms::SetVec3(const char *name, const float *v3)
    {
        const auto &e = Lookup(name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(mProgram, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(mProgram, name, GL_FLOAT_VEC3, e.type);
        glUniform3fv(e.location, 1, v3);
    }

    void ProgramUniforms::SetVec2(const char *name, const float *v2)
    {
        const auto &e = Lookup(name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(mProgram, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(mProgram, name, GL_FLOAT_VEC2, e.type);
        glUniform2fv(e.location, 1, v2);
    }

    void ProgramUniforms::SetFloat(const char *name, float v)
    {
        const auto &e = Lookup(name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(mProgram, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(mProgram, name, GL_FLOAT, e.type);
        glUniform1f(e.location, v);
    }

    void ProgramUniforms::SetInt(const char *name, int v)
    {
        const auto &e = Lookup(name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(mProgram, name);
            return;
        }
        // GL_INT / GL_SAMPLER_2D / GL_SAMPLER_CUBE 등 모두 glUniform1i 로 설정 ->
        // 단일 expected 값으로 검증 어려워 타입 검증 생략.
        glUniform1i(e.location, v);
    }

    GLint ProgramUniforms::Get(const char *name)
    {
        const auto &e = Lookup(name);
        if (e.location < 0)
            Diagnostics::UniformDiagnostics::NotifyMissing(mProgram, name);
        return e.location;
    }
}
