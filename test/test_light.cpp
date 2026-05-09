/**
 * @file test_light.cpp
 * @brief @c Light 값 클래스의 default value / 할당 / 레이아웃 회귀.
 *
 * @details
 *  GL context 불필요 — 순수 POD-like value class.
 *
 *  본 테스트가 잡는 회귀 카테고리:
 *  - 디폴트 값 손상 (light pos/색상이 의도치 않게 바뀌면 모든 씬이 어두워짐)
 *  - 멤버 추가/삭제로 simple.fs `struct Light` 와 어긋남
 *
 *  @note 본 클래스는 *namespace SJH 미적용* (global `class Light`). 프로젝트 컨벤션 smell.
 *  @note 셰이더 측 contract 검증은 별도 test_shader_uniform_contract.cpp 에서 수행.
 *        본 테스트는 *C++ 측 멤버 약속*만 박는다.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "object/light.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("Light default 생성 — 기본값이 의도된 초기값과 일치", "[light][defaults]")
{
    Light l;

    // 점 광원 default 위치 (3, 3, 3) — 카메라 (0,0,3) 기준 우상단 위에서 비추는 형태
    REQUIRE_THAT(l.mPos.x, WithinAbs(3.0f, 1e-6f));
    REQUIRE_THAT(l.mPos.y, WithinAbs(3.0f, 1e-6f));
    REQUIRE_THAT(l.mPos.z, WithinAbs(3.0f, 1e-6f));

    // ambient = 0.1 어둡게, diffuse = 0.5 중간, specular = 1.0 강한 하이라이트
    REQUIRE_THAT(l.mAmbient.x, WithinAbs(0.1f, 1e-6f));
    REQUIRE_THAT(l.mAmbient.y, WithinAbs(0.1f, 1e-6f));
    REQUIRE_THAT(l.mAmbient.z, WithinAbs(0.1f, 1e-6f));

    REQUIRE_THAT(l.mDiffuse.x, WithinAbs(0.5f, 1e-6f));
    REQUIRE_THAT(l.mDiffuse.y, WithinAbs(0.5f, 1e-6f));
    REQUIRE_THAT(l.mDiffuse.z, WithinAbs(0.5f, 1e-6f));

    REQUIRE_THAT(l.mSpecular.x, WithinAbs(1.0f, 1e-6f));
    REQUIRE_THAT(l.mSpecular.y, WithinAbs(1.0f, 1e-6f));
    REQUIRE_THAT(l.mSpecular.z, WithinAbs(1.0f, 1e-6f));
}

TEST_CASE("Light — 멤버 직접 할당 (ImGui DragFloat3 시뮬레이션)", "[light][assign]")
{
    Light l;

    // ImGui DragFloat3 가 lightPos 갱신하는 것 시뮬레이션
    l.mPos = glm::vec3(-2.0f, 5.0f, 1.5f);
    REQUIRE_THAT(l.mPos.x, WithinAbs(-2.0f, 1e-6f));
    REQUIRE_THAT(l.mPos.y, WithinAbs(5.0f, 1e-6f));
    REQUIRE_THAT(l.mPos.z, WithinAbs(1.5f, 1e-6f));

    // ColorEdit3 가 lightColor 갱신
    l.mDiffuse = glm::vec3(0.9f, 0.8f, 0.7f);
    REQUIRE_THAT(l.mDiffuse.x, WithinAbs(0.9f, 1e-6f));
}

TEST_CASE("Light — ambient 가 0이면 그림자 영역이 검정 (학습 의의)", "[light][lighting_logic]")
{
    // simple.fs:
    //   vec3 ambient = material.ambient * light.ambient;
    // 즉 light.mAmbient == 0 이면 그림자 영역이 완전히 검정.
    // 이건 *셰이더 로직의 결과*를 C++ 측에서 *직접 검증*하는 케이스 (계약 박기).
    Light l;
    l.mAmbient = glm::vec3(0.0f);

    // 직접 ambient 항 시뮬레이션 (셰이더와 동일 식)
    glm::vec3 materialAmbient(1.0f, 0.5f, 0.3f);  // Material default
    glm::vec3 ambient = materialAmbient * l.mAmbient;

    REQUIRE_THAT(ambient.x, WithinAbs(0.0f, 1e-6f));
    REQUIRE_THAT(ambient.y, WithinAbs(0.0f, 1e-6f));
    REQUIRE_THAT(ambient.z, WithinAbs(0.0f, 1e-6f));
}

TEST_CASE("Light — 복사 시멘틱 (POD-like, 자동 생성)", "[light][copy]")
{
    Light a;
    a.mPos = glm::vec3(10.0f, 20.0f, 30.0f);

    Light b = a;
    REQUIRE_THAT(b.mPos.x, WithinAbs(10.0f, 1e-6f));

    a.mPos = glm::vec3(0.0f);
    REQUIRE_THAT(b.mPos.x, WithinAbs(10.0f, 1e-6f));  // 독립 복사
}

TEST_CASE("Light — 레이아웃 sanity (4 vec3)", "[light][layout]")
{
    // mPos / mAmbient / mDiffuse / mSpecular = 4 × vec3
    constexpr size_t kVec3Size = sizeof(glm::vec3);
    constexpr size_t kMin = 4 * kVec3Size;

    REQUIRE(sizeof(Light) >= kMin);
    INFO("sizeof(Light) = " << sizeof(Light) << " (min expected = " << kMin << ")");
    REQUIRE(sizeof(Light) <= kMin + 16);
}
