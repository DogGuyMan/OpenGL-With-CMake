/**
 * @file program_uniforms.cpp
 * @brief @c SJH::Uniforms 자유 함수 family 구현 — *Program 외부의* 정적 캐시 보유.
 *
 * @details
 *  ### 캐시 자료구조
 *  @code{.cpp}
 *    std::unordered_map<GLuint, std::unordered_map<std::string, UniformEntry>>
 *    //                  ^^^^^ ProgramInstanceId    ^^^^^^^^^^^ name -> entry
 *  @endcode
 *  - 외부 키 = @c GLuint (Program::GetProgramAddr() 결과).
 *  - 내부 = name -> entry (이전 디자인의 1-level 맵과 동일).
 *  - 본 TU 의 anonymous namespace 에 static — *프로세스 lifetime*. 학습 프로젝트 규모에서
 *    Program 마다 수십 entry × 수 KB 수준 -> 메모리 leak 부담 없음.
 *
 *  ### Program 과의 의존성
 *  - 본 TU 만 @c program/program.h 를 include — Program::GetProgramAddr() 의 *public* getter 만 호출.
 *  - 헤더 (@c program_uniforms.h) 는 forward declaration 만 사용 — Program 정의 의존 X.
 */

#include "program/program.h"
#include "program/program_uniforms.h"
#include "diagnostics/uniform_diagnostics.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace SJH::Uniforms
{
    namespace
    {
        using NameToEntry = std::unordered_map<std::string, UniformEntry>;

        // === Program 외부에 위치, GLuint 키로 조회 ===
        // ProgramInstanceId -> (name -> entry).
        std::unordered_map<GLuint, NameToEntry> sCacheRegistry;

        /// @brief 캐시 lookup + miss 시 lazy 보강. 부수효과 없음 (warn 등 진단은 진입점이 위임).
        const UniformEntry &LookupOrInsert(GLuint program, NameToEntry &cache, const char *name)
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

    // === 진입점들 — 모두 prog.GetProgramAddr() (public) 만 사용. friend 권한 불필요. ===

    void BuildCache(Program &prog)
    {
        const GLuint pid = prog.GetProgramAddr();
        auto &cache = sCacheRegistry[pid];   // operator[] 가 없으면 default 생성
        cache.clear();

        GLint count = 0;
        glGetProgramiv(pid, GL_ACTIVE_UNIFORMS, &count);
        if (count <= 0)
            return;

        GLint maxNameLen = 0;
        glGetProgramiv(pid, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLen);
        if (maxNameLen <= 0)
            return;

        std::vector<char> nameBuf(static_cast<size_t>(maxNameLen + 1), '\0');
        for (GLint i = 0; i < count; ++i)
        {
            GLsizei nameSize = 0;
            GLint   size     = 0;
            GLenum  type     = 0;
            glGetActiveUniform(pid, static_cast<GLuint>(i),
                               maxNameLen, &nameSize, &size, &type, nameBuf.data());

            const GLint loc = glGetUniformLocation(pid, nameBuf.data());
            cache.emplace(
                std::string(nameBuf.data(), static_cast<size_t>(nameSize)),
                UniformEntry{loc, type});
        }
    }

    void Forget(GLuint programId)
    {
        sCacheRegistry.erase(programId);
    }

    void SetMat4(Program &prog, const char *name, const float *mat4)
    {
        const GLuint pid = prog.GetProgramAddr();
        const auto &e = LookupOrInsert(pid, sCacheRegistry[pid], name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(pid, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(pid, name, GL_FLOAT_MAT4, e.type);
        glUniformMatrix4fv(e.location, 1, GL_FALSE, mat4);
    }

    void SetVec4(Program &prog, const char *name, const float *v4)
    {
        const GLuint pid = prog.GetProgramAddr();
        const auto &e = LookupOrInsert(pid, sCacheRegistry[pid], name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(pid, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(pid, name, GL_FLOAT_VEC4, e.type);
        glUniform4fv(e.location, 1, v4);
    }

    void SetVec3(Program &prog, const char *name, const float *v3)
    {
        const GLuint pid = prog.GetProgramAddr();
        const auto &e = LookupOrInsert(pid, sCacheRegistry[pid], name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(pid, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(pid, name, GL_FLOAT_VEC3, e.type);
        glUniform3fv(e.location, 1, v3);
    }

    void SetVec2(Program &prog, const char *name, const float *v2)
    {
        const GLuint pid = prog.GetProgramAddr();
        const auto &e = LookupOrInsert(pid, sCacheRegistry[pid], name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(pid, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(pid, name, GL_FLOAT_VEC2, e.type);
        glUniform2fv(e.location, 1, v2);
    }

    void SetFloat(Program &prog, const char *name, float v)
    {
        const GLuint pid = prog.GetProgramAddr();
        const auto &e = LookupOrInsert(pid, sCacheRegistry[pid], name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(pid, name);
            return;
        }
        Diagnostics::UniformDiagnostics::NotifyTypeMismatch(pid, name, GL_FLOAT, e.type);
        glUniform1f(e.location, v);
    }

    void SetInt(Program &prog, const char *name, int v)
    {
        const GLuint pid = prog.GetProgramAddr();
        const auto &e = LookupOrInsert(pid, sCacheRegistry[pid], name);
        if (e.location < 0)
        {
            Diagnostics::UniformDiagnostics::NotifyMissing(pid, name);
            return;
        }
        // GL_INT / GL_SAMPLER_2D / GL_SAMPLER_CUBE 등 모두 glUniform1i 로 설정 ->
        // 단일 expected 값으로 검증 어려워 타입 검증 생략.
        glUniform1i(e.location, v);
    }

    GLint Get(Program &prog, const char *name)
    {
        const GLuint pid = prog.GetProgramAddr();
        const auto &e = LookupOrInsert(pid, sCacheRegistry[pid], name);
        if (e.location < 0)
            Diagnostics::UniformDiagnostics::NotifyMissing(pid, name);
        return e.location;
    }
}
