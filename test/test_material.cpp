/**
 * @file test_material.cpp
 * @brief @c Material 값 클래스의 default value / 할당 / 레이아웃 회귀.
 *
 * @details
 *  GL context 불필요 — 순수 POD-like value class.
 *
 *  본 테스트가 잡는 회귀 카테고리:
 *  - 디폴트 값 손상 (예: mShininess 32 → 0 으로 잘못 변경 시 specular 무력화)
 *  - 멤버 추가/삭제로 [resources/shader/simple.fs] 의 `struct Material` 과 어긋남
 *    (셰이더 contract 검증은 별도 test_shader_uniform_contract.cpp)
 *
 *  @note 본 클래스는 *namespace SJH 미적용* (global `class Material`). 프로젝트 컨벤션과
 *        다른 점은 인지된 smell — 본 테스트는 *현재 상태*를 박는 characterization 테스트.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "shader/material.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("Material default 생성 — 기본값이 의도된 초기값과 일치", "[material][defaults]")
{
    Material m;

    // simple.fs 의 struct Material { vec3 ambient; vec3 diffuse; vec3 specular; float shininess; }
    // 기본값 — 셰이더 정의와 무관하게 *C++ 측 기본값 동결* (characterization).
    REQUIRE_THAT(m.mAmbient.x,  WithinAbs(1.0f, 1e-6f));
    REQUIRE_THAT(m.mAmbient.y,  WithinAbs(0.5f, 1e-6f));
    REQUIRE_THAT(m.mAmbient.z,  WithinAbs(0.3f, 1e-6f));

    REQUIRE_THAT(m.mDiffuse.x,  WithinAbs(1.0f, 1e-6f));
    REQUIRE_THAT(m.mDiffuse.y,  WithinAbs(0.5f, 1e-6f));
    REQUIRE_THAT(m.mDiffuse.z,  WithinAbs(0.3f, 1e-6f));

    REQUIRE_THAT(m.mSpecular.x, WithinAbs(0.5f, 1e-6f));
    REQUIRE_THAT(m.mSpecular.y, WithinAbs(0.5f, 1e-6f));
    REQUIRE_THAT(m.mSpecular.z, WithinAbs(0.5f, 1e-6f));

    REQUIRE_THAT(m.mShininess,  WithinAbs(32.0f, 1e-6f));
}

TEST_CASE("Material — 멤버 직접 할당 + 읽기", "[material][assign]")
{
    Material m;
    m.mAmbient   = glm::vec3(0.1f, 0.2f, 0.3f);
    m.mDiffuse   = glm::vec3(0.4f, 0.5f, 0.6f);
    m.mSpecular  = glm::vec3(0.7f, 0.8f, 0.9f);
    m.mShininess = 64.0f;

    REQUIRE_THAT(m.mAmbient.x, WithinAbs(0.1f, 1e-6f));
    REQUIRE_THAT(m.mDiffuse.y, WithinAbs(0.5f, 1e-6f));
    REQUIRE_THAT(m.mSpecular.z, WithinAbs(0.9f, 1e-6f));
    REQUIRE_THAT(m.mShininess, WithinAbs(64.0f, 1e-6f));
}

TEST_CASE("Material — 복사 시멘틱 (POD-like, 자동 생성)", "[material][copy]")
{
    Material a;
    a.mShininess = 128.0f;

    Material b = a;   // copy
    REQUIRE_THAT(b.mShininess, WithinAbs(128.0f, 1e-6f));

    // 원본은 변경 없음
    a.mShininess = 1.0f;
    REQUIRE_THAT(b.mShininess, WithinAbs(128.0f, 1e-6f));
}

TEST_CASE("Material — 레이아웃 sanity (3 vec3 + 1 float)", "[material][layout]")
{
    // glm::vec3 는 3 float 이지만 alignment 때문에 실제 sizeof 는 12 bytes (또는 16).
    // 본 테스트는 *Material 의 멤버 추가/삭제*를 catch하기 위함이지 ABI/UBO 호환을 보장하는 것이 아님.
    // UBO 호환 보장은 std140 alignment 명시 + 별도 시리얼라이즈 테스트가 담당해야 함.

    constexpr size_t kVec3Size = sizeof(glm::vec3);  // 보통 12, 환경 따라 16 가능
    constexpr size_t kFloatSize = sizeof(float);     // 4

    // 3 × vec3 + 1 × float ≤ Material ≤ 3 × vec3 + 1 × float + padding
    constexpr size_t kMin = 3 * kVec3Size + kFloatSize;
    REQUIRE(sizeof(Material) >= kMin);

    // 멤버가 추가되면 본 단언이 깨짐 — 의도된 추가 시 본 케이스도 갱신 필요
    INFO("sizeof(Material) = " << sizeof(Material) << " (min expected = " << kMin << ")");
    REQUIRE(sizeof(Material) <= kMin + 16);  // 합리적 padding 상한
}
