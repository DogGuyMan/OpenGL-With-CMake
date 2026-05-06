/**
 * @file test_uniform_diagnostics.cpp
 * @brief @c SJH::Diagnostics::UniformDiagnostics 의 warn-once 트래커 smoke 테스트.
 *
 * @details
 *  - 본 클래스는 spdlog 로그만 발생 (GL 호출 X) — GL context fixture 불필요.
 *  - spdlog sink 캡처 없이는 *warn 발생 여부* 를 직접 단언하기 어려우므로
 *    smoke 테스트 중심: 호출이 crash 없이 완료되고 Invalidate 이후 안전한지.
 *  - 기능 자체의 *동작* 은 사용자가 stderr 로 spdlog 출력을 보며 확인.
 */

#include <catch2/catch_test_macros.hpp>

#include "diagnostics/uniform_diagnostics.h"

TEST_CASE("UniformDiagnostics::NotifyMissing 다중 호출 안전", "[diagnostics][uniform]")
{
    using SJH::Diagnostics::UniformDiagnostics;

    // (program=42, name="uA") 를 두 번 — 두 번째는 warn-once 로 silent.
    UniformDiagnostics::NotifyMissing(42, "uMissingA");
    UniformDiagnostics::NotifyMissing(42, "uMissingA");

    // 같은 program, 다른 name — 새로운 warn.
    UniformDiagnostics::NotifyMissing(42, "uMissingB");

    // 다른 program, 같은 name — 다른 키, warn 발생.
    UniformDiagnostics::NotifyMissing(99, "uMissingA");

    SUCCEED("NotifyMissing 다중 호출 crash 없음");
}

TEST_CASE("UniformDiagnostics::NotifyTypeMismatch 시나리오", "[diagnostics][uniform]")
{
    using SJH::Diagnostics::UniformDiagnostics;
    constexpr GLuint kProgram = 42;

    // 불일치 — warn (첫 호출만).
    UniformDiagnostics::NotifyTypeMismatch(kProgram, "uMat", GL_FLOAT_MAT4, GL_FLOAT_VEC4);
    UniformDiagnostics::NotifyTypeMismatch(kProgram, "uMat", GL_FLOAT_MAT4, GL_FLOAT_VEC4); // silent

    // 일치 — silent.
    UniformDiagnostics::NotifyTypeMismatch(kProgram, "uOk", GL_FLOAT, GL_FLOAT);

    // actual==0 (active 정보 없음, lazy 보강 케이스) — silent (검증 skip).
    UniformDiagnostics::NotifyTypeMismatch(kProgram, "uUnknown", GL_FLOAT, 0);

    SUCCEED("NotifyTypeMismatch 모든 시나리오 crash 없음");
}

TEST_CASE("UniformDiagnostics::Invalidate 멱등 + 미존재 program 안전", "[diagnostics][uniform]")
{
    using SJH::Diagnostics::UniformDiagnostics;

    constexpr GLuint kProgram = 7;

    UniformDiagnostics::NotifyMissing(kProgram, "uX");
    UniformDiagnostics::Invalidate(kProgram);
    UniformDiagnostics::Invalidate(kProgram);   // 두 번 호출 — 멱등 (idempotent)
    UniformDiagnostics::Invalidate(99999);      // 트래커에 없는 program — 안전

    // Invalidate 후 같은 (program, name) warn 다시 발생 가능 검증 (이름 중복 트래커 클리어).
    UniformDiagnostics::NotifyMissing(kProgram, "uX");

    SUCCEED("Invalidate 멱등 + 미존재 program 안전");
}
