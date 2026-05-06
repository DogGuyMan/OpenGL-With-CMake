/**
 * @file uniform_diagnostics.cpp
 * @brief @c UniformDiagnostics 구현 — 내부 program-키 글로벌 트래커.
 */

#include "uniform_diagnostics.h"

#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

// 공개 불능시키고 사용하기.
namespace
{
    namespace detail
    {
        std::unordered_map<GLuint, std::unordered_set<std::string>> warnedMissing;
        std::unordered_map<GLuint, std::unordered_set<std::string>> warnedTypeMismatch;
    }
}

namespace SJH::Diagnostics
{
    void UniformDiagnostics::NotifyMissing(GLuint program, const char *name)
    {
        if (detail::warnedMissing[program].insert(name).second)
        {
            spdlog::warn("uniform missing in program {}: '{}'", program, name);
        }
    }

    void UniformDiagnostics::NotifyTypeMismatch(GLuint program, const char *name,
                                                GLenum expected, GLenum actual)
    {
        if (actual == 0)
            return; // active 정보 없음 (lazy 보강 케이스) — 검증 skip
        if (actual == expected)
            return; // 일치 -> no-op

        if (detail::warnedTypeMismatch[program].insert(name).second)
        {
            spdlog::warn("uniform type mismatch in program {} '{}': expected 0x{:x}, actual 0x{:x}",
                         program, name, expected, actual);
        }
    }

    void UniformDiagnostics::Invalidate(GLuint program)
    {
        detail::warnedMissing.erase(program);
        detail::warnedTypeMismatch.erase(program);
    }
}
