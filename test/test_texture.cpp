/**
 * @file test_texture.cpp
 * @brief @c SJH::Image / @c SJH::Texture / @c SJH::ResourceManagement 검증.
 *
 * @details
 *  ### 영역
 *  - **Image**: 파일 로드 성공/실패, dimensions/data 유효성.
 *  - **Texture**: Image 로부터 GL 텍스처 생성, @c Bind 의 상태 머신 변경, 이동 의미론.
 *  - **ResourceManagement**: 팩토리 (CreateRM), 두 캐시 (images/textures) 의 히트/미스/Clear.
 *  - **GL 계약**: sampler uniform 은 @c glUseProgram 이후에만 적용된다는 spec 검증.
 *
 *  ### 비영역
 *  - 실제 렌더 파이프라인 통합 — "텍스처가 화면에 *보이는가*" 는 Phase B3/B4 픽셀 readback 영역.
 *  - 빌드 신선도 (stale binary) 같은 *환경* 결함은 본 테스트로 잡지 못함 — 빌드 위생 별도 영역.
 *
 *  ### 회귀 가드 (`[regression]` 태그)
 *  - @c CreateRM 이 *빈* @c unique_ptr 을 반환하지 않는지 — null deref 회귀 방지.
 *  - @c Texture::operator= 가 source 의 핸들을 0 으로 비우는지 — double-delete 회귀 방지.
 *  - @c glUniform1i 가 @c glUseProgram 전에 호출되면 silently 실패 — sampler 미설정 회귀 방지.
 *
 *  ### 테스트 자원 요구
 *  - @c ./resources/texture/container.jpg 가 *실행 디렉토리* 에 있다고 가정.
 *    없으면 SKIP (빌드 시 symlink/copy 가 동작했는지 확인).
 */

#include <catch2/catch_test_macros.hpp>

#include "support/gl_test_fixture.h"
#include "resource_management/resource_management.h"

#include <glad/glad.h>

#include <filesystem>
#include <utility>

namespace fs = std::filesystem;

namespace
{
    const fs::path  kSampleImage = "./resources/texture/container.jpg";
    constexpr auto  kImageName   = "container";
    constexpr auto  kMissingPath = "./resources/texture/__definitely_not_existing__.png";

    bool SampleImageAvailable()
    {
        std::error_code ec;
        return fs::exists(kSampleImage, ec);
    }
}

// =====================================================================
// Image  — 파일 로드 + 메타데이터
// =====================================================================

TEST_CASE("Image::Load — 정상 경로에서 dimensions/data 유효", "[image]")
{
    if (!SampleImageAvailable()) SKIP("샘플 이미지 없음: " << kSampleImage);

    auto image = SJH::Image::Load(kSampleImage.string(), kImageName);
    REQUIRE(image != nullptr);
    REQUIRE(image->GetWidth()       > 0);
    REQUIRE(image->GetHeight()      > 0);
    REQUIRE(image->GetChannelCount() > 0);
    REQUIRE(image->GetDataPtr()    != nullptr);
    REQUIRE(image->GetImageName()  == kImageName);
}

TEST_CASE("Image::Load — 미존재 경로는 nullptr", "[image]")
{
    auto image = SJH::Image::Load(kMissingPath, "x");
    REQUIRE(image == nullptr);
}

// =====================================================================
// Texture  — GL 핸들 생성 + 상태 머신
// =====================================================================

TEST_CASE("Texture::CreateTexture — Image 로부터 GL 텍스처 생성, ID 유효",
          "[texture][gl]")
{
    if (!SampleImageAvailable()) SKIP("샘플 이미지 없음");
    SJH::test::GLContextFixture ctx;

    auto image = SJH::Image::Load(kSampleImage.string(), kImageName);
    REQUIRE(image != nullptr);

    auto texture = SJH::Texture::CreateTexture(image.get());
    REQUIRE(texture != nullptr);
    REQUIRE(texture->GetTextureID() != 0);
}

TEST_CASE("Texture::Bind — GL_TEXTURE_BINDING_2D 가 인스턴스 핸들을 가리킨다",
          "[texture][gl]")
{
    if (!SampleImageAvailable()) SKIP("샘플 이미지 없음");
    SJH::test::GLContextFixture ctx;

    auto image   = SJH::Image::Load(kSampleImage.string(), kImageName);
    auto texture = SJH::Texture::CreateTexture(image.get());
    REQUIRE(texture->GetTextureID() != 0);

    // 외부 텍스처를 먼저 바인딩 — Bind 가 *전환* 하는지 확인.
    GLuint externalTex = 0;
    glGenTextures(1, &externalTex);
    glBindTexture(GL_TEXTURE_2D, externalTex);

    GLint boundBefore = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundBefore);
    REQUIRE(static_cast<GLuint>(boundBefore) == externalTex);

    texture->Bind();

    GLint boundAfter = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundAfter);
    REQUIRE(static_cast<GLuint>(boundAfter) == texture->GetTextureID());

    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &externalTex);
}

/**
 * @brief 회귀 가드 — @c Texture::operator= 의 이동 의미론.
 *
 * @details 이동 후 source 의 @c mTextureID 는 반드시 0 이어야 함. 그렇지 않으면 source 소멸 시
 *  자기가 이미 양도한 핸들을 한 번 더 @c glDeleteTextures 하여 sink 의 텍스처를 *몰래 삭제* —
 *  이후 GL 이 같은 ID 를 다른 텍스처에 재할당하면 sink 의 Bind 가 *남의 텍스처* 를 가리키게 됨.
 */
TEST_CASE("Texture::operator= (move) — source 의 핸들이 0 으로 비워져야 함",
          "[texture][gl][move][regression]")
{
    if (!SampleImageAvailable()) SKIP("샘플 이미지 없음");
    SJH::test::GLContextFixture ctx;

    auto image = SJH::Image::Load(kSampleImage.string(), kImageName);
    auto t1    = SJH::Texture::CreateTexture(image.get());
    auto t2    = SJH::Texture::CreateTexture(image.get());

    REQUIRE(t1->GetTextureID() != 0);
    REQUIRE(t2->GetTextureID() != 0);
    REQUIRE(t1->GetTextureID() != t2->GetTextureID());

    const GLuint t2_originalId = t2->GetTextureID();

    *t1 = std::move(*t2);   // Texture::operator=(Texture&&) 호출

    // 이동 후 sink 는 source 의 ID 를 이어받아야 함.
    REQUIRE(t1->GetTextureID() == t2_originalId);
    // 이동의 의무 — source 는 핸들 0 으로 비워져야 함.
    // 비워지지 않으면 t2 소멸자가 t1 의 텍스처를 다시 delete → silent corruption.
    REQUIRE(t2->GetTextureID() == 0);
}

// =====================================================================
// ResourceManagement  — 팩토리 + 두 캐시
// =====================================================================

/**
 * @brief 회귀 가드 — @c CreateRM 이 *빈* unique_ptr 을 반환해선 안 됨.
 *
 * @details 흔한 실수: @c std::unique_ptr<RM>() (default ctor) 으로 빈 포인터를 만들면
 *  반환값은 항상 nullptr → 모든 호출자에서 즉시 null deref. 본 테스트는 단 한 줄로 그 회귀를 잡음.
 */
TEST_CASE("ResourceManagement::CreateRM — 유효한 인스턴스 반환",
          "[rm][regression]")
{
    auto rm = SJH::ResourceManagement::CreateRM();
    REQUIRE(rm != nullptr);
}

TEST_CASE("ResourceManagement::LoadImage — 같은 image_name 캐시 히트", "[rm]")
{
    if (!SampleImageAvailable()) SKIP("샘플 이미지 없음");
    SJH::test::GLContextFixture ctx;

    auto rm = SJH::ResourceManagement::CreateRM();
    REQUIRE(rm != nullptr);

    auto* first  = rm->LoadImage(kSampleImage.string(), kImageName);
    auto* second = rm->LoadImage(kSampleImage.string(), kImageName);

    REQUIRE(first  != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first == second);   // 캐시 히트 — 동일 인스턴스 반환
}

TEST_CASE("ResourceManagement::LoadImage — 미존재 경로는 nullptr", "[rm]")
{
    SJH::test::GLContextFixture ctx;
    auto rm = SJH::ResourceManagement::CreateRM();
    REQUIRE(rm != nullptr);

    auto* image = rm->LoadImage(kMissingPath, "x");
    REQUIRE(image == nullptr);
}

TEST_CASE("ResourceManagement::LoadTextureFromImage — 같은 Image 두 번 → 캐시 히트",
          "[rm][texture][gl]")
{
    if (!SampleImageAvailable()) SKIP("샘플 이미지 없음");
    SJH::test::GLContextFixture ctx;

    auto rm = SJH::ResourceManagement::CreateRM();
    REQUIRE(rm != nullptr);

    auto* image = rm->LoadImage(kSampleImage.string(), kImageName);
    REQUIRE(image != nullptr);

    auto* tex1 = rm->LoadTextureFromImage(image);
    auto* tex2 = rm->LoadTextureFromImage(image);

    REQUIRE(tex1 != nullptr);
    REQUIRE(tex2 != nullptr);
    REQUIRE(tex1 == tex2);   // 캐시 키 = image_name → 동일 인스턴스
    REQUIRE(tex1->GetTextureID() == tex2->GetTextureID());
}

TEST_CASE("ResourceManagement::LoadTextureWithName — 미로드 미스 / 로드 후 히트",
          "[rm][texture][gl]")
{
    if (!SampleImageAvailable()) SKIP("샘플 이미지 없음");
    SJH::test::GLContextFixture ctx;

    auto rm = SJH::ResourceManagement::CreateRM();
    REQUIRE(rm != nullptr);

    // 미로드 — nullptr.
    REQUIRE(rm->LoadTextureWithName(kImageName) == nullptr);

    // 텍스처 로드 후 같은 이름으로 lookup — 히트.
    auto* image = rm->LoadImage(kSampleImage.string(), kImageName);
    REQUIRE(image != nullptr);
    auto* tex = rm->LoadTextureFromImage(image);
    REQUIRE(tex != nullptr);

    auto* lookup = rm->LoadTextureWithName(kImageName);
    REQUIRE(lookup == tex);
}

/**
 * @brief @c Clear 는 두 캐시를 모두 비운다 — lookup 미스로 검증.
 *
 * @note *주소 비교* 로 "다른 객체" 를 단언하지 않는 것이 정석:
 *  Clear 후 새 emplace 시 allocator 가 freelist 의 *같은 슬롯* 을 재사용해 raw 포인터가
 *  우연히 동일해질 수 있음. dangling-pointer-reuse 함정 — 단언이 *조용히 통과* 해 회귀를 놓침.
 *  Clear 의 진짜 계약은 "캐시 항목이 사라진다" → @c LoadTextureWithName 이 미스를 반환.
 */
TEST_CASE("ResourceManagement::Clear — 두 캐시 모두 비움 (lookup 미스로 검증)",
          "[rm][texture][gl]")
{
    if (!SampleImageAvailable()) SKIP("샘플 이미지 없음");
    SJH::test::GLContextFixture ctx;

    auto rm = SJH::ResourceManagement::CreateRM();
    REQUIRE(rm != nullptr);

    auto* image = rm->LoadImage(kSampleImage.string(), kImageName);
    REQUIRE(image != nullptr);
    REQUIRE(rm->LoadTextureFromImage(image) != nullptr);

    // Clear 전 — 텍스처 캐시 히트.
    REQUIRE(rm->LoadTextureWithName(kImageName) != nullptr);

    rm->Clear();

    // Clear 후 — 같은 이름 lookup 미스 (캐시 비어있음).
    REQUIRE(rm->LoadTextureWithName(kImageName) == nullptr);

    // 재로드 가능 — 정상 동작 복귀.
    auto* image2 = rm->LoadImage(kSampleImage.string(), kImageName);
    REQUIRE(image2 != nullptr);
    REQUIRE(rm->LoadTextureFromImage(image2) != nullptr);
}

// =====================================================================
// GL 계약 회귀 가드  — sampler uniform 호출 순서
// =====================================================================

/**
 * @brief 회귀 가드 — @c glUniform* 호출은 program use 후에만 적용됨.
 *
 * @details core profile spec: 어떤 program 도 use 중이지 않을 때 @c glUniform* 호출은
 *  @c GL_INVALID_OPERATION 발생 + uniform 값 *변경 없음*. @c glGetError() 폴링이 없으면
 *  외관상 silent 처럼 보여 sampler 가 default 0 에 머무는 단색 회귀가 가능.
 */
TEST_CASE("Sampler uniform — Use() 전 glUniform1i 는 INVALID_OPERATION + 값 미반영",
          "[texture][gl][regression]")
{
    SJH::test::GLContextFixture ctx;

    const char* vsSrc =
        "#version 330 core\n"
        "void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }\n";
    const char* fsSrc =
        "#version 330 core\n"
        "uniform sampler2D tex;\n"
        "out vec4 frag;\n"
        "void main() { frag = texture(tex, vec2(0.0)); }\n";

    auto compile = [](const char* src, GLenum type) {
        GLuint sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, nullptr);
        glCompileShader(sh);
        return sh;
    };
    GLuint vs = compile(vsSrc, GL_VERTEX_SHADER);
    GLuint fs = compile(fsSrc, GL_FRAGMENT_SHADER);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint linked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    REQUIRE(linked == GL_TRUE);

    GLint loc = glGetUniformLocation(prog, "tex");
    REQUIRE(loc >= 0);

    // === 케이스 A — Use() 전 glUniform1i (버그 패턴) ===
    glUseProgram(0);
    while (glGetError() != GL_NO_ERROR) {} // 에러 큐 비움
    glUniform1i(loc, 7);
    REQUIRE(glGetError() == GL_INVALID_OPERATION);

    GLint sampledBefore = -1;
    glUseProgram(prog);
    glGetUniformiv(prog, loc, &sampledBefore);
    REQUIRE(sampledBefore == 0);   // 설정 실패 — 디폴트 0 유지

    // === 케이스 B — Use() 후 glUniform1i (정상 패턴) ===
    while (glGetError() != GL_NO_ERROR) {}
    glUniform1i(loc, 3);
    REQUIRE(glGetError() == GL_NO_ERROR);

    GLint sampledAfter = -1;
    glGetUniformiv(prog, loc, &sampledAfter);
    REQUIRE(sampledAfter == 3);   // 정상 설정됨

    glUseProgram(0);
    glDeleteProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
}
