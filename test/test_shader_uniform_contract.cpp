/**
 * @file test_shader_uniform_contract.cpp
 * @brief @c resources/shader/simple.fs 소스 ↔ C++ @c Material / @c Light 클래스의 *contract* 검증.
 *
 * @details
 *  ### 동기
 *  C++ 측 @c Material / @c Light 클래스가 셰이더의 @c struct Material / @c struct Light 와
 *  *필드 단위로 일치*해야 uniform 업로드가 정상 동작. 어긋나면:
 *  - 셰이더가 기대하는 uniform 이 *그 이름으로* 존재하지 않아 location -1 (UniformDiagnostics 잡음)
 *  - 또는 *이름이 매치하나 타입 불일치* (UniformDiagnostics::NotifyTypeMismatch 잡음)
 *  - 최악: 둘 다 매치하지만 *값이 안 들어가서* 까만색 / specular 무력화 (audit D4/D5)
 *
 *  본 테스트는 셰이더 *소스 텍스트*를 읽어 정규식으로 검증 — GL 컴파일 불필요, 빠름.
 *
 *  ### 잡는 회귀
 *  - simple.fs 의 `#version` 이 macOS GL 4.1 상한 초과 (BugReport §3 — A1 회귀)
 *  - simple.fs 가 `uniform Light light;` / `uniform Material material;` 선언을 잃음
 *  - struct 필드 이름이 C++ 측 멤버 (m-prefix 제거 후 lowercase) 와 어긋남
 *  - 핵심 free-floating uniform (`baseColor`, `tex0`, `tex1`, `viewPos`) 누락
 *
 *  ### 한계
 *  - 정규식 기반이라 주석 안 코드 / preprocessor 분기까지는 못 봄
 *  - 셰이더 *시맨틱*은 검증 X (예: ambient 계산식이 올바른지)
 *  - GL 컴파일 검증은 별도 — `Shader::CreateFromFile` 호출 후 GLObjectLog::CheckShaderCompile
 *
 * @see [resources/shader/simple.fs](../resources/shader/simple.fs)
 * @see [bug-coverage-audit.md](../doc/testplan/bug-coverage-audit.md) 카테고리 A, D, J
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <fstream>
#include <regex>
#include <sstream>
#include <string>

using Catch::Matchers::ContainsSubstring;

namespace
{
    /// CMake 가 SJH_RESOURCES_DIR 매크로로 절대 경로 주입 — 빌드 디렉토리 위치와 무관하게 접근.
    std::string LoadFile(const std::string &relPath)
    {
        const std::string fullPath = std::string(SJH_RESOURCES_DIR) + "/" + relPath;
        std::ifstream fin(fullPath);
        if (!fin.is_open())
        {
            FAIL("cannot open shader file: " << fullPath);
        }
        std::stringstream ss;
        ss << fin.rdbuf();
        return ss.str();
    }
}

// ──────────────────────────────────────────────────────────────────────────
// macOS GL 4.1 상한 검증 — BugReport.md §3 회귀 방지
// ──────────────────────────────────────────────────────────────────────────

TEST_CASE("simple.fs — #version 이 410 이하인지 (macOS GL 4.1 상한)",
          "[shader_contract][version]")
{
    auto src = LoadFile("shader/simple.fs");

    // #version XYY 형식 추출
    std::regex versionRe(R"(#version\s+(\d+)\s+core)");
    std::smatch m;
    REQUIRE(std::regex_search(src, m, versionRe));

    const int version = std::stoi(m[1].str());
    INFO("simple.fs declared version: " << version);
    // BugReport §3 회귀 방지 — #version 430 사건이 다시 일어나면 본 단언이 깨짐.
    REQUIRE(version <= 410);
    REQUIRE(version >= 330);  // 너무 낮아도 의도와 다름 (현재 330)
}

// ──────────────────────────────────────────────────────────────────────────
// uniform Light light; ↔ C++ Light 클래스
// ──────────────────────────────────────────────────────────────────────────

TEST_CASE("simple.fs — uniform Light light 선언 존재", "[shader_contract][light]")
{
    auto src = LoadFile("shader/simple.fs");
    REQUIRE_THAT(src, ContainsSubstring("uniform Light light"));
    REQUIRE_THAT(src, ContainsSubstring("struct Light"));
}

TEST_CASE("simple.fs — struct Light 필드 (position/ambient/diffuse/specular)",
          "[shader_contract][light]")
{
    auto src = LoadFile("shader/simple.fs");

    // C++ Light::mPos → shader light.position (의도된 *semantic rename*)
    REQUIRE_THAT(src, ContainsSubstring("vec3 position"));
    // C++ Light::mAmbient → shader light.ambient (m 제거)
    REQUIRE_THAT(src, ContainsSubstring("vec3 ambient"));
    REQUIRE_THAT(src, ContainsSubstring("vec3 diffuse"));
    REQUIRE_THAT(src, ContainsSubstring("vec3 specular"));
}

TEST_CASE("simple.fs — light 사용 표현 (lightDir / specular / diffuse 식)",
          "[shader_contract][light]")
{
    auto src = LoadFile("shader/simple.fs");

    // light.position 이 normalize 식에서 사용
    REQUIRE_THAT(src, ContainsSubstring("light.position"));
    // ambient 항 곱셈
    REQUIRE_THAT(src, ContainsSubstring("light.ambient"));
}

// ──────────────────────────────────────────────────────────────────────────
// uniform Material material; ↔ C++ Material 클래스
// ──────────────────────────────────────────────────────────────────────────

TEST_CASE("simple.fs — uniform Material material 선언 존재",
          "[shader_contract][material]")
{
    auto src = LoadFile("shader/simple.fs");
    REQUIRE_THAT(src, ContainsSubstring("uniform Material material"));
    REQUIRE_THAT(src, ContainsSubstring("struct Material"));
}

TEST_CASE("simple.fs — struct Material 필드 (ambient/diffuse/specular/shininess)",
          "[shader_contract][material]")
{
    auto src = LoadFile("shader/simple.fs");

    // C++ Material::mAmbient/mDiffuse/mSpecular → shader material.ambient/...
    REQUIRE_THAT(src, ContainsSubstring("ambient"));
    REQUIRE_THAT(src, ContainsSubstring("diffuse"));
    REQUIRE_THAT(src, ContainsSubstring("specular"));

    // C++ Material::mShininess float → shader material.shininess float
    REQUIRE_THAT(src, ContainsSubstring("float shininess"));
    REQUIRE_THAT(src, ContainsSubstring("material.shininess"));
}

// ──────────────────────────────────────────────────────────────────────────
// Free-floating uniforms — Context::Render 가 직접 setter 호출하는 항목
// ──────────────────────────────────────────────────────────────────────────

TEST_CASE("simple.fs — free-floating uniforms (baseColor/tex0/tex1/viewPos) 선언",
          "[shader_contract][free_uniform]")
{
    auto src = LoadFile("shader/simple.fs");

    // context.cpp Render() 가 직접 SetVec4/SetInt/SetVec3 로 호출하는 uniform
    REQUIRE_THAT(src, ContainsSubstring("uniform vec4 baseColor"));
    REQUIRE_THAT(src, ContainsSubstring("uniform sampler2D tex0"));
    REQUIRE_THAT(src, ContainsSubstring("uniform sampler2D tex1"));
    REQUIRE_THAT(src, ContainsSubstring("uniform vec3 viewPos"));
}

TEST_CASE("simple.fs — fragColor 출력 + main() 진입점 존재",
          "[shader_contract][entry]")
{
    auto src = LoadFile("shader/simple.fs");

    REQUIRE_THAT(src, ContainsSubstring("out vec4 fragColor"));
    REQUIRE_THAT(src, ContainsSubstring("void main()"));
}

// ──────────────────────────────────────────────────────────────────────────
// 부정 — 의도치 않은 토큰이 없어야 함 (회귀 가시화)
// ──────────────────────────────────────────────────────────────────────────

TEST_CASE("simple.fs — bool 비트연산 `|` (GLSL 컴파일 실패 패턴) 없음",
          "[shader_contract][negative]")
{
    // STUDY_NOTE Ex6 §1-5: bool 에 `|` 사용 → 컴파일 실패. 본 셰이더에 없는지 확인.
    auto src = LoadFile("shader/simple.fs");

    // bool 식 `if (a < b | c < d)` 패턴 — `||` 가 아닌 단일 `|` 만 잡기
    std::regex badBitOr(R"(\)\s*\|\s*[a-zA-Z_])");  // ") | <ident>" — 비트 OR + 식별자
    REQUIRE_FALSE(std::regex_search(src, badBitOr));
}

TEST_CASE("simple.fs — `out VS_OUT` 같은 FS interface block 방향 오류 없음",
          "[shader_contract][negative]")
{
    // STUDY_NOTE Ex6 §1-1: FS 가 out VS_OUT 으로 선언하면 입력 못 받음.
    // 본 셰이더는 in vec3/vec2/vec4 단일 변수 사용 (interface block 미사용) → 자동 통과.
    auto src = LoadFile("shader/simple.fs");
    REQUIRE_THAT(src, ContainsSubstring("in vec3 vsNormal"));
    REQUIRE_FALSE(src.find("out VS_OUT") != std::string::npos);
}
