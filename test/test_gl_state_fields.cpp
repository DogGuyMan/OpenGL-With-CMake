/**
 * @file test_gl_state_fields.cpp
 * @brief SymbolicName 사전 적중/미적중/GL_ZERO 정책 검증.
 * @details GL context 불필요 — 순수 함수 테스트.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "diagnostics/gl_state_fields.h"
#include <glad/glad.h>

using SJH::Diagnostics::SymbolicName;
using Catch::Matchers::Equals;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("SymbolicName 사전 적중", "[diagnostics][state_fields]")
{
    REQUIRE_THAT(SymbolicName(GL_LESS),    Equals("GL_LESS"));
    REQUIRE_THAT(SymbolicName(GL_LEQUAL),  Equals("GL_LEQUAL"));
    REQUIRE_THAT(SymbolicName(GL_BACK),    Equals("GL_BACK"));
    REQUIRE_THAT(SymbolicName(GL_FRONT),   Equals("GL_FRONT"));
    REQUIRE_THAT(SymbolicName(GL_CCW),     Equals("GL_CCW"));
    REQUIRE_THAT(SymbolicName(GL_CW),      Equals("GL_CW"));
    REQUIRE_THAT(SymbolicName(GL_ONE),     Equals("GL_ONE"));
    REQUIRE_THAT(SymbolicName(GL_SRC_ALPHA), Equals("GL_SRC_ALPHA"));
}

TEST_CASE("SymbolicName(0) → GL_ZERO 정책 (blend factor 컨텍스트)", "[diagnostics][state_fields]")
{
    // 본 프로젝트 17개 캡처 필드 한정 시 0이 enum 값으로 합법 발생하는 곳은
    // blend_src_rgb / blend_dst_rgb 뿐 — GL_ZERO 가 정확.
    // 미래 GL_TEXTURE_COMPARE_MODE 등 GL_NONE-context 추가 시 필드별 분기 (현재 YAGNI).
    REQUIRE_THAT(SymbolicName(0), Equals("GL_ZERO"));
}

TEST_CASE("SymbolicName 미적중 → hex fallback", "[diagnostics][state_fields]")
{
    REQUIRE_THAT(SymbolicName(0xDEAD),  Equals("0xDEAD"));
    REQUIRE_THAT(SymbolicName(0xBEEF),  Equals("0xBEEF"));
    // 4자리 hex (대문자) — 사람이 매뉴얼 검색 가능한 형식
    REQUIRE_THAT(SymbolicName(0xABCD),  Equals("0xABCD"));
}

TEST_CASE("SymbolicName GL_TEXTUREn 동적 표현", "[diagnostics][state_fields]")
{
    // active_texture 필드는 GL_TEXTURE0..GL_TEXTURE15 범위 — 사전 16개 등록 비효율,
    // 동적으로 "GL_TEXTURE{n}" 포맷.
    REQUIRE_THAT(SymbolicName(GL_TEXTURE0),  Equals("GL_TEXTURE0"));
    REQUIRE_THAT(SymbolicName(GL_TEXTURE1),  Equals("GL_TEXTURE1"));
    REQUIRE_THAT(SymbolicName(GL_TEXTURE15), Equals("GL_TEXTURE15"));
}

TEST_CASE("SymbolicName 사전 boundary — depth_func 모든 8개", "[diagnostics][state_fields]")
{
    REQUIRE_THAT(SymbolicName(GL_NEVER),    Equals("GL_NEVER"));
    REQUIRE_THAT(SymbolicName(GL_LESS),     Equals("GL_LESS"));
    REQUIRE_THAT(SymbolicName(GL_EQUAL),    Equals("GL_EQUAL"));
    REQUIRE_THAT(SymbolicName(GL_LEQUAL),   Equals("GL_LEQUAL"));
    REQUIRE_THAT(SymbolicName(GL_GREATER),  Equals("GL_GREATER"));
    REQUIRE_THAT(SymbolicName(GL_NOTEQUAL), Equals("GL_NOTEQUAL"));
    REQUIRE_THAT(SymbolicName(GL_GEQUAL),   Equals("GL_GEQUAL"));
    REQUIRE_THAT(SymbolicName(GL_ALWAYS),   Equals("GL_ALWAYS"));
}
