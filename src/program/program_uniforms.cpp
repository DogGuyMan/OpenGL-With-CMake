/**
 * @file program_uniforms.cpp
 * @brief @c SJH::Uniforms 자유 함수 family 구현 — Program 의 private 캐시 (friend) 접근.
 */

#include "program/program.h"
#include "program/program_uniforms.h"
#include "diagnostics/uniform_diagnostics.h"

#include <vector>

namespace SJH::Uniforms
{
    namespace
    {
        // === 내부 헬퍼 — Lookup ===
        // friend 권한은 호출 진입점 (BuildCache / Set* / Get) 에서만 가지므로,
        // 내부 헬퍼는 인자로 캐시 ref 를 받아 같은 컴파일 단위 안에서 재사용.
        // 부수효과 없음 (warn 등 진단은 진입점이 위임).
        const UniformEntry & LookupOrInsert(GLuint program,
                        std::unordered_map<std::string, UniformEntry> &cache,
                        const char *name)
        {
            const auto it = cache.find(name);
            if (it != cache.end())
                return it->second;

            // Cache miss — 배열 원소 같은 비-canonical 이름일 수 있음. lazy 보강.
            // 배열 원소 type 은 active 인덱스 탐색 필요 — 진단 가치 < 비용이라 type 은 0 유지.
            UniformEntry e;
            e.location = glGetUniformLocation(program, name);
            return cache.emplace(name, e).first->second;
        }
    }

    // === 진입점들 — Program 의 friend 권한으로 private 멤버 (mProgramAddr / mUniformCache) 접근 ===

    void BuildCache(Program &prog)
    {
        prog.mUniformCache.clear();

        GLint count = 0;
        glGetProgramiv(prog.mProgramAddr, GL_ACTIVE_UNIFORMS, &count);
        if (count <= 0)
            return;

        GLint maxNameLen = 0;
        glGetProgramiv(prog.mProgramAddr, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLen);
        if (maxNameLen <= 0)
            return;

        std::vector<char> nameBuf(static_cast<size_t>(maxNameLen + 1), '\0');
        for (GLint i = 0; i < count; ++i)
        {
            GLsizei nameSize = 0;
            GLint   size     = 0;
            GLenum  type     = 0;
            glGetActiveUniform(prog.mProgramAddr, static_cast<GLuint>(i),
                               maxNameLen, &nameSize, &size, &type, nameBuf.data());

            const GLint loc = glGetUniformLocation(prog.mProgramAddr, nameBuf.data());
            prog.mUniformCache.emplace(
                std::string(nameBuf.data(), static_cast<size_t>(nameSize)),
                UniformEntry{loc, type});
        }
    }

    void SetMat4(Program &prog, const char *name, const float *mat4)
    {
        const auto &e = LookupOrInsert(prog.mProgramAddr, prog.mUniformCache, name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(prog.mProgramAddr, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(prog.mProgramAddr, name,
                                                            GL_FLOAT_MAT4, e.type);
        glUniformMatrix4fv(e.location, 1, GL_FALSE, mat4);
    }

    void SetVec4(Program &prog, const char *name, const float *v4)
    {
        const auto &e = LookupOrInsert(prog.mProgramAddr, prog.mUniformCache, name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(prog.mProgramAddr, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(prog.mProgramAddr, name,
                                                            GL_FLOAT_VEC4, e.type);
        glUniform4fv(e.location, 1, v4);
    }

    void SetVec3(Program &prog, const char *name, const float *v3)
    {
        const auto &e = LookupOrInsert(prog.mProgramAddr, prog.mUniformCache, name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(prog.mProgramAddr, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(prog.mProgramAddr, name,
                                                            GL_FLOAT_VEC3, e.type);
        glUniform3fv(e.location, 1, v3);
    }

    void SetVec2(Program &prog, const char *name, const float *v2)
    {
        const auto &e = LookupOrInsert(prog.mProgramAddr, prog.mUniformCache, name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(prog.mProgramAddr, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(prog.mProgramAddr, name,
                                                            GL_FLOAT_VEC2, e.type);
        glUniform2fv(e.location, 1, v2);
    }

    void SetFloat(Program &prog, const char *name, float v)
    {
        const auto &e = LookupOrInsert(prog.mProgramAddr, prog.mUniformCache, name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(prog.mProgramAddr, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(prog.mProgramAddr, name,
                                                            GL_FLOAT, e.type);
        glUniform1f(e.location, v);
    }

    void SetInt(Program &prog, const char *name, int v)
    {
        const auto &e = LookupOrInsert(prog.mProgramAddr, prog.mUniformCache, name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(prog.mProgramAddr, name);
            return;
        }
        // GL_INT / GL_SAMPLER_2D / GL_SAMPLER_CUBE 등 모두 glUniform1i 로 설정 →
        // 단일 expected 값으로 검증 어려워 타입 검증 생략.
        glUniform1i(e.location, v);
    }

    GLint Get(Program &prog, const char *name)
    {
        const auto &e = LookupOrInsert(prog.mProgramAddr, prog.mUniformCache, name);
        if (e.location < 0)
            Diagnostics::UniformDiagnostics::NotifyMissing(prog.mProgramAddr, name);
        return e.location;
    }
}
