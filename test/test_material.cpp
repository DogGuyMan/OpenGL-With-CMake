/**
 * @file test_material.cpp
 * @brief @c Material 값 클래스의 default value / setter 불변식 / copy 시멘틱 회귀.
 *
 * @details
 *  GL context 불필요 — 순수 value class (@c GLuint/@c GLint 는 타입 별칭일 뿐, GL 호출 없음).
 *
 *  본 테스트가 잡는 회귀 카테고리:
 *  - 디폴트 값 손상 (예: @c mShininess 32 → 0 으로 잘못 변경 시 specular 무력화).
 *  - 이름 setter 의 *핸들 무효화* 누락 — 이름과 핸들이 어긋난 stale 상태.
 *  - @c SetShininess 의 clamp 범위 ([2, 256]) 손상.
 *
 *  @note 본 클래스는 *namespace SJH 미적용* (global @c class Material). 컨벤션과 다른 점은
 *        인지된 smell — 향후 SJH:: 이동 예정 (material.h 의 @note 참조).
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "shader/material.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("Material default 생성 — 기본값이 의도된 초기값과 일치", "[material][defaults]")
{
    Material m;

    REQUIRE(m.GetDiffuseTextureName().empty());
    REQUIRE(m.GetSpecularTextureName().empty());
    REQUIRE(m.GetDiffuseTexture()  == 0);
    REQUIRE(m.GetSpecularTexture() == 0);
    REQUIRE(m.GetDiffuseUnit()  == 0);
    REQUIRE(m.GetSpecularUnit() == 1);
    REQUIRE_THAT(m.GetShininess(), WithinAbs(32.0f, 1e-6f));
    REQUIRE_FALSE(m.IsResolved());   // 핸들 0 이면 미해석
}

TEST_CASE("Material::SetTextureNames — 두 이름 동시 설정, 핸들은 미해석(0) 유지",
          "[material][names]")
{
    Material m;
    m.SetTextureNames("container2", "container2_specular");

    REQUIRE(m.GetDiffuseTextureName()  == "container2");
    REQUIRE(m.GetSpecularTextureName() == "container2_specular");
    // 이름만 설정 — 아직 resolve 안 함 → 핸들 0.
    REQUIRE(m.GetDiffuseTexture()  == 0);
    REQUIRE(m.GetSpecularTexture() == 0);
    REQUIRE_FALSE(m.IsResolved());
}

TEST_CASE("Material — 이름 변경 시 해석된 핸들 *무효화*", "[material][names][invalidation]")
{
    Material m;
    m.SetTextureNames("container2", "container2_specular");
    m.SetResolvedTextures(/*diffuse*/ 10, /*diffUnit*/ 2, /*specular*/ 11, /*specUnit*/ 3);

    REQUIRE(m.IsResolved());
    REQUIRE(m.GetDiffuseTexture()  == 10);
    REQUIRE(m.GetSpecularTexture() == 11);

    // 디퓨즈 이름만 바꾸면 → 디퓨즈 핸들 0 으로 무효화. 스페큘러는 그대로.
    m.SetDiffuseTextureName("wood");
    REQUIRE(m.GetDiffuseTextureName() == "wood");
    REQUIRE(m.GetDiffuseTexture()  == 0);   // 무효화됨
    REQUIRE(m.GetSpecularTexture() == 11);  // 영향 없음
    REQUIRE_FALSE(m.IsResolved());           // 디퓨즈 핸들 0 → 미해석으로 간주

    // 스페큘러 이름 변경도 동일.
    m.SetSpecularTextureName("metal");
    REQUIRE(m.GetSpecularTexture() == 0);
}

TEST_CASE("Material::SetResolvedTextures — 핸들 + 유닛 동시 설정", "[material][resolve]")
{
    Material m;
    m.SetResolvedTextures(/*diffuse*/ 42, /*diffUnit*/ 5, /*specular*/ 43, /*specUnit*/ 6);

    REQUIRE(m.GetDiffuseTexture()  == 42);
    REQUIRE(m.GetSpecularTexture() == 43);
    REQUIRE(m.GetDiffuseUnit()  == 5);
    REQUIRE(m.GetSpecularUnit() == 6);
    REQUIRE(m.IsResolved());
}

TEST_CASE("Material::SetShininess — [2, 256] 으로 clamp", "[material][shininess]")
{
    Material m;

    SECTION("범위 내 값은 그대로")
    {
        m.SetShininess(64.0f);
        REQUIRE_THAT(m.GetShininess(), WithinAbs(64.0f, 1e-6f));
    }
    SECTION("하한 미만은 2 로 clamp")
    {
        m.SetShininess(0.0f);
        REQUIRE_THAT(m.GetShininess(), WithinAbs(2.0f, 1e-6f));
        m.SetShininess(-100.0f);
        REQUIRE_THAT(m.GetShininess(), WithinAbs(2.0f, 1e-6f));
    }
    SECTION("상한 초과는 256 으로 clamp")
    {
        m.SetShininess(1000.0f);
        REQUIRE_THAT(m.GetShininess(), WithinAbs(256.0f, 1e-6f));
    }
    SECTION("경계값은 통과")
    {
        m.SetShininess(2.0f);
        REQUIRE_THAT(m.GetShininess(), WithinAbs(2.0f, 1e-6f));
        m.SetShininess(256.0f);
        REQUIRE_THAT(m.GetShininess(), WithinAbs(256.0f, 1e-6f));
    }
}

TEST_CASE("Material — 복사 시멘틱 (자동 생성, 깊은 복사)", "[material][copy]")
{
    Material a;
    a.SetTextureNames("d", "s");
    a.SetResolvedTextures(7, 1, 8, 2);
    a.SetShininess(128.0f);

    Material b = a;   // copy
    REQUIRE(b.GetDiffuseTextureName()  == "d");
    REQUIRE(b.GetSpecularTextureName() == "s");
    REQUIRE(b.GetDiffuseTexture()  == 7);
    REQUIRE(b.GetSpecularTexture() == 8);
    REQUIRE_THAT(b.GetShininess(), WithinAbs(128.0f, 1e-6f));

    // 원본 변경이 복사본에 영향 없음.
    a.SetShininess(2.0f);
    a.SetDiffuseTextureName("changed");
    REQUIRE_THAT(b.GetShininess(), WithinAbs(128.0f, 1e-6f));
    REQUIRE(b.GetDiffuseTextureName() == "d");
}
