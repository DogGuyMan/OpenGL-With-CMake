# ResourceRegistry 리팩토링 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 리소스 소유권을 이름·타입·호출지점에 드러내도록 `resource_management` 모듈을 `ResourceRegistry`로 재정비한다.

**Architecture:** 6개 원자적 Task. 리팩토링이므로 각 Task의 검증은 "빌드 green + 기존 ctest 전체 통과"이며, 신규 동작(`Clone`/`CreateModel`/`FindModel`)에만 신규 테스트를 추가한다. Task 경계마다 빌드가 깨지지 않도록 순서를 잡았다 — 모듈/클래스 리네임 → API 동사 분리 → Material `const Texture*` → `shared_ptr` 제거 → Model 전역 캐시 → 문서.

**Tech Stack:** C++17, CMake + Ninja, vcpkg, Catch2, OpenGL/glad, assimp, spdlog

**설계 출처:** [doc/design/2026-05-16-resource-registry-design.md](2026-05-16-resource-registry-design.md)

**공통 빌드/테스트 명령:**
```bash
# CMakeLists/파일 리네임 후엔 재구성 필요 (vcpkg 스킵)
cmake --preset debug -DVCPKG_MANIFEST_INSTALL=OFF
# 코드만 수정한 경우
cmake --build build_Darwin
# 테스트
ctest --test-dir build_Darwin --output-on-failure
```

---

## Task 1: 모듈/클래스 리네임 (mechanical, 동작 변화 없음)

`resource_management` → `resource_registry`, 클래스 `ResourceManagement` → `ResourceRegistry`.
순수 리네임 — 이 Task 종료 시 동작은 완전히 동일하고 모든 테스트가 그대로 통과해야 한다.

**Files:**
- Rename(git mv): `src/resource_management/` → `src/resource_registry/`
- Rename(git mv): `resource_management.h` → `resource_registry.h`, `resource_management.cpp` → `resource_registry.cpp`
- Modify: `src/resource_registry/CMakeLists.txt`, `src/CMakeLists.txt`, `src/context/CMakeLists.txt`, `app/CMakeLists.txt`, `test/CMakeLists.txt`
- Modify: `src/resource_registry/resource_registry.h`, `resource_registry.cpp`, `texture.h`, `texture.cpp`
- Modify: `src/context/context.h`, `src/context/context.cpp`, `src/object/model.h`, `src/object/model.cpp`, `src/shader/material.h`
- Modify: `test/test_texture.cpp`

- [ ] **Step 1: 디렉터리/파일 git mv**

```bash
git mv src/resource_management src/resource_registry
git mv src/resource_registry/resource_management.h src/resource_registry/resource_registry.h
git mv src/resource_registry/resource_management.cpp src/resource_registry/resource_registry.cpp
```

- [ ] **Step 2: `src/resource_registry/CMakeLists.txt` 수정**

타겟·alias·소스 파일명을 교체:

```cmake
add_library(sjhopengl_resource_registry STATIC
    resource_registry.cpp
    texture.cpp
    image.cpp
)
add_library(SJH::resource_registry ALIAS sjhopengl_resource_registry)

target_include_directories(sjhopengl_resource_registry
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${Stb_INCLUDE_DIR}
)

target_link_libraries(sjhopengl_resource_registry
    PUBLIC  SJH::common
            SJH::object
            glad::glad
            glm::glm
            spdlog::spdlog
    PRIVATE SJH::diagnostics
)

target_compile_features(sjhopengl_resource_registry PRIVATE cxx_std_17)
```

- [ ] **Step 3: 소비 CMakeLists 갱신**

`src/CMakeLists.txt` — `add_subdirectory(resource_management)` → `add_subdirectory(resource_registry)`.
`src/context/CMakeLists.txt` — `SJH::resource_management` → `SJH::resource_registry`.
`app/CMakeLists.txt` — `SJH::resource_management` → `SJH::resource_registry` (31번째 줄).
`test/CMakeLists.txt` — `test_texture` 의 `target_link_libraries` 안 `SJH::resource_management` → `SJH::resource_registry`, 그리고 주석 `#  resource_management 모듈` → `#  resource_registry 모듈`.

- [ ] **Step 4: 헤더 가드 + include 경로 + 클래스명 일괄 치환**

다음 토큰을 전 파일에서 치환한다:

| 대상 | 변경 전 | 변경 후 |
|---|---|---|
| 헤더 가드 (`resource_registry.h`) | `__SJH_RESOURCE_MANAGEMENT_H__` | `__SJH_RESOURCE_REGISTRY_H__` |
| include 경로 | `"resource_management/resource_management.h"` | `"resource_registry/resource_registry.h"` |
| include 경로 | `"resource_management/texture.h"` | `"resource_registry/texture.h"` |
| include 경로 | `"resource_management/image.h"` | `"resource_registry/image.h"` |
| `resource_registry.cpp` 의 include | `#include "resource_management.h"` | `#include "resource_registry.h"` |
| `texture.cpp` 의 include | `#include "resource_management.h"` | `#include "resource_registry.h"` |
| 클래스명 | `ResourceManagement` | `ResourceRegistry` |
| `CLASS_PTR` 매크로 인자 | `CLASS_PTR(ResourceManagement)` | `CLASS_PTR(ResourceRegistry)` |
| 스마트 포인터 별칭 | `ResourceManagementUPtr` | `ResourceRegistryUPtr` |
| 팩토리 메서드 | `CreateRM` | `Create` |

영향 파일: `resource_registry.h`(가드·`CLASS_PTR`·클래스·`CreateRM`), `resource_registry.cpp`(`ResourceManagement::`·`CreateRM`), `texture.h`(`friend class ResourceManagement`·include), `texture.cpp`(include), `context.h`(include·`ResourceManagementUPtr mRM`), `context.cpp`(`ResourceManagement::CreateRM()` → `ResourceRegistry::Create()`), `model.h`(include), `model.cpp`(include), `material.h`(doxygen 주석의 `ResourceManagement` 언급), `test_texture.cpp`(include·`SJH::ResourceManagement`·`CreateRM`·파일 상단 doxygen).

`texture.h:33` `friend class ResourceManagement;` → 일단 `friend class ResourceRegistry;` 로 *리네임만* (제거는 Step 6).

- [ ] **Step 5: 재구성 + 빌드 + 테스트**

Run:
```bash
cmake --preset debug -DVCPKG_MANIFEST_INSTALL=OFF
cmake --build build_Darwin
ctest --test-dir build_Darwin --output-on-failure
```
Expected: 빌드 성공, 모든 테스트 통과 (리네임만 했으므로 동작 불변).

- [ ] **Step 6: `friend` 제거 시도**

`src/resource_registry/texture.h` 에서 `friend class ResourceRegistry;` 줄을 삭제한다.
근거: `ResourceRegistry` 는 public `Texture::CreateTexture` 만 사용하므로 friend 불필요
(architecture.md §11.1 "friend 0개" 진화).

Run: `cmake --build build_Darwin`
Expected: PASS. 만약 컴파일 실패하면 friend가 실제로 필요한 것이므로 줄을 복구한다.

- [ ] **Step 7: 커밋**

```bash
git add -A
git commit -m "[refactor] : resource_management 모듈을 resource_registry 로 리네임

ResourceManagement → ResourceRegistry, CreateRM → Create.
디렉터리·CMake 타겟·alias·include 경로 전부 통일. friend 제거.
동작 변화 없음 — 전체 테스트 통과.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
```

---

## Task 2: API 동사 분리 (Create / Find) + `images` 맵 제거 + `Clear()` 수정

`Load*` 동사를 `Create*`(생성) / `Find*`(조회)로 엄격 분리한다. `images` 캐시를 제거한다.
이 Task는 `Material` 시그니처를 건드리지 않는다 (`GLuint` 세계 유지) — Task 3에서 변경.

**Files:**
- Modify: `src/resource_registry/resource_registry.h`, `src/resource_registry/resource_registry.cpp`
- Modify: `src/context/context.cpp`
- Modify: `test/test_texture.cpp`

- [ ] **Step 1: `resource_registry.h` API 선언 교체**

`public:` 메서드 선언을 다음으로 교체한다 (`LoadImage` 삭제, 동사 교체, `FindMaterial` 추가):

```cpp
        /// @brief 매니저 인스턴스 팩토리.
        static ResourceRegistryUPtr Create();

        /// @brief 보유 자원 일괄 해제 후 매니저 자체 소멸.
        ~ResourceRegistry();

        /// @brief Image 로부터 GPU 텍스처를 *생성*하고 @p key 로 캐시. 이미 있으면 실패(nullptr).
        Texture *CreateTexture(const std::string &key, const Image *image);

        /// @brief @p key 로 캐시된 텍스처 *조회* (생성 안 함). 없으면 nullptr.
        Texture *FindTexture(const std::string &key);

        /// @brief 파일에서 디퓨즈 텍스처를 로드해 Material 을 *생성*하고 @p key 로 캐시. 이미 있으면 실패(nullptr).
        Material *CreateMaterial(const std::string &key, const std::string &filepath);

        /// @brief @p key 로 캐시된 머티리얼 *조회* (생성 안 함). 없으면 nullptr.
        Material *FindMaterial(const std::string &key);

        /// @brief 보유 모든 자원 일괄 해제 (매니저 인스턴스 자체는 유지).
        void Clear();
```

`private:` 멤버에서 `images` 맵 줄을 **삭제**한다:

```cpp
    private:
        std::unordered_map<std::string, TextureUPtr> textures;
        std::unordered_map<std::string, MaterialPtr> materials;
        std::unordered_map<std::string, ModelUPtr> models;
        ResourceRegistry() = default;
```

`#include "image.h"` 는 `CreateTexture` 인자 타입(`const Image*`)에 필요하므로 유지한다.

- [ ] **Step 2: `resource_registry.cpp` 본문 재작성**

`LoadImage` / `LoadTextureWithName` / `LoadTextureFromImage` / `LoadMaterial` 정의를 다음으로 교체한다:

```cpp
    Texture *ResourceRegistry::CreateTexture(const std::string &key, const Image *image)
    {
        if (textures.find(key) != textures.end())
        {
            spdlog::warn("CreateTexture: 키 '{}' 가 이미 존재 — Find 를 먼저 호출하라", key);
            return nullptr;
        }
        auto texture = Texture::CreateTexture(image);
        if (texture == nullptr)
            return nullptr;
        auto [insertedIt, ok] = textures.emplace(key, std::move(texture));
        return insertedIt->second.get();
    }

    Texture *ResourceRegistry::FindTexture(const std::string &key)
    {
        auto it = textures.find(key);
        return (it != textures.end()) ? it->second.get() : nullptr;
    }

    Material *ResourceRegistry::CreateMaterial(const std::string &key, const std::string &filepath)
    {
        if (materials.find(key) != materials.end())
        {
            spdlog::warn("CreateMaterial: 키 '{}' 가 이미 존재 — Find 를 먼저 호출하라", key);
            return nullptr;
        }

        auto image = Image::Load(key, filepath);   // 스코프 한정 — 이 함수 끝에서 소멸
        if (image == nullptr)
            return nullptr;

        auto texture = Texture::CreateTexture(image.get());
        if (texture == nullptr)
            return nullptr;
        const GLuint diffuseHandle = texture->GetTextureID();
        // 텍스처를 registry 캐시에 보관 — Material 핸들의 lifetime owner (co-location).
        textures.emplace(key, std::move(texture));

        auto material = Material::Create();
        material->SetDiffuseTextureName(key);
        material->SetResolvedTextures(diffuseHandle, /*diffuseUnit*/ 0,
                                      /*specular*/ 0u, /*specularUnit*/ 1);

        auto [insertedIt, ok] = materials.emplace(key, std::move(material));
        return insertedIt->second.get();
    }

    Material *ResourceRegistry::FindMaterial(const std::string &key)
    {
        auto it = materials.find(key);
        return (it != materials.end()) ? it->second.get() : nullptr;
    }
```

`Clear()` 에 `models.clear()` 를 추가하고 `images.clear()` 를 제거한다:

```cpp
    void ResourceRegistry::Clear()
    {
        textures.clear();
        materials.clear();
        models.clear();
    }
```

`resource_registry.cpp` 상단에 `#include <spdlog/spdlog.h>` 가 없으면 추가한다 (`spdlog::warn` 사용).

- [ ] **Step 3: `context.cpp` `Init()` 의 텍스처 로딩 블록 재작성**

`mRM = ResourceRegistry::Create();` 줄부터 `mMaterial->SetResolvedTextures(...)` 끝까지(현재 325~371행)를 다음으로 교체한다:

```cpp
        mRM = ResourceRegistry::Create();

        // 이미지는 스코프 한정 — CreateTexture 가 GPU 업로드를 마치면 즉시 소멸.
        struct TexSpec { const char *key; const char *path; };
        const TexSpec fileTextures[] = {
            {"container",           "./resources/texture/container.jpg"},
            {"awesomeface",         "./resources/texture/awesomeface.png"},
            {"container2",          "./resources/texture/container2.png"},
            {"container2_specular", "./resources/texture/container2_specular.png"},
        };
        for (const auto &spec : fileTextures)
        {
            auto image = Image::Load(spec.key, spec.path);
            if (image == nullptr)
                return false;
            mRM->CreateTexture(spec.key, image.get());
        }

        auto checkerImg = Image::Create("checkerboard", 512, 512);
        if (checkerImg == nullptr)
            return false;
        checkerImg->SetCheckImage(16, 16);
        mRM->CreateTexture("checkerboard", checkerImg.get());

        auto whiteImg = Image::Create("white", 32, 32);
        if (whiteImg == nullptr)
            return false;
        whiteImg->SetWhiteImage();
        mRM->CreateTexture("white", whiteImg.get());

        auto grayImg = Image::Create("gray", 32, 32);
        if (grayImg == nullptr)
            return false;
        grayImg->SetSingleColorImage({0.5f, 0.5f, 0.5f, 1.0f});
        mRM->CreateTexture("gray", grayImg.get());

        mMaterial = Material::Create();
        mMaterial->SetTextureNames("white", "gray");
        mMaterial->SetResolvedTextures(
            mRM->FindTexture("white")->GetTextureID(), 0,
            mRM->FindTexture("gray")->GetTextureID(), 1);
```

- [ ] **Step 4: `test_texture.cpp` 의 RM 테스트 케이스 재작성**

`ResourceManagement — 팩토리 + 두 캐시` 섹션의 테스트들을 교체한다.

(a) `"ResourceManagement::LoadImage — 같은 image_name 캐시 히트"` 와
`"ResourceManagement::LoadImage — 미존재 경로는 nullptr"` 두 TEST_CASE 를 **삭제** (images 캐시 제거).

(b) `"ResourceManagement::LoadTextureFromImage — 같은 Image 두 번 -> 캐시 히트"` 를 교체:

```cpp
TEST_CASE("ResourceRegistry::CreateTexture — 생성 후 FindTexture 가 같은 인스턴스 반환",
          "[rm][texture][gl]")
{
    if (!SampleImageAvailable()) SKIP("샘플 이미지 없음");
    SJH::test::GLContextFixture ctx;

    auto rm = SJH::ResourceRegistry::Create();
    REQUIRE(rm != nullptr);

    auto image = SJH::Image::Load(kImageName, kSampleImage.string());
    REQUIRE(image != nullptr);

    auto* created = rm->CreateTexture(kImageName, image.get());
    REQUIRE(created != nullptr);

    // 같은 키로 CreateTexture 재호출 — 엄격 분리: 실패(nullptr).
    REQUIRE(rm->CreateTexture(kImageName, image.get()) == nullptr);

    // 조회는 FindTexture — 같은 인스턴스.
    REQUIRE(rm->FindTexture(kImageName) == created);
}
```

(c) `"ResourceManagement::LoadTextureWithName — 미로드 미스 / 로드 후 히트"` 를 교체:

```cpp
TEST_CASE("ResourceRegistry::FindTexture — 미생성 미스 / 생성 후 히트",
          "[rm][texture][gl]")
{
    if (!SampleImageAvailable()) SKIP("샘플 이미지 없음");
    SJH::test::GLContextFixture ctx;

    auto rm = SJH::ResourceRegistry::Create();
    REQUIRE(rm != nullptr);

    REQUIRE(rm->FindTexture(kImageName) == nullptr);   // 미생성 — 미스

    auto image = SJH::Image::Load(kImageName, kSampleImage.string());
    REQUIRE(image != nullptr);
    auto* tex = rm->CreateTexture(kImageName, image.get());
    REQUIRE(tex != nullptr);

    REQUIRE(rm->FindTexture(kImageName) == tex);        // 생성 후 — 히트
}
```

(d) `"ResourceManagement::Clear — 두 캐시 모두 비움"` 을 교체:

```cpp
TEST_CASE("ResourceRegistry::Clear — 캐시를 비움 (FindTexture 미스로 검증)",
          "[rm][texture][gl]")
{
    if (!SampleImageAvailable()) SKIP("샘플 이미지 없음");
    SJH::test::GLContextFixture ctx;

    auto rm = SJH::ResourceRegistry::Create();
    REQUIRE(rm != nullptr);

    auto image = SJH::Image::Load(kImageName, kSampleImage.string());
    REQUIRE(image != nullptr);
    REQUIRE(rm->CreateTexture(kImageName, image.get()) != nullptr);
    REQUIRE(rm->FindTexture(kImageName) != nullptr);

    rm->Clear();
    REQUIRE(rm->FindTexture(kImageName) == nullptr);

    // 재생성 가능 — 정상 복귀.
    auto image2 = SJH::Image::Load(kImageName, kSampleImage.string());
    REQUIRE(image2 != nullptr);
    REQUIRE(rm->CreateTexture(kImageName, image2.get()) != nullptr);
}
```

파일 상단 doxygen 주석의 `두 캐시 (images/textures)` 표현을 `텍스처 캐시` 로 수정한다.

- [ ] **Step 5: 빌드 + 테스트**

Run:
```bash
cmake --build build_Darwin
ctest --test-dir build_Darwin --output-on-failure
```
Expected: 빌드 성공, 모든 테스트 통과.

- [ ] **Step 6: 커밋**

```bash
git add -A
git commit -m "[refactor] : ResourceRegistry API 를 Create/Find 로 엄격 분리

Load* 3중 의미 해소 — CreateTexture/FindTexture/CreateMaterial/FindMaterial.
images 캐시 제거 (Image 는 스코프 한정). Clear() 가 models 도 비우도록 수정.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
```

---

## Task 3: `Material` — `const Texture*` 관찰자 + `Clone()`

`Material` 이 raw `GLuint` 대신 비소유 `const Texture*` 관찰자를 들도록 한다. `Clone()` 추가.

**Files:**
- Modify: `src/shader/material.h`
- Modify: `src/resource_registry/resource_registry.cpp`, `src/object/model.cpp`, `src/context/context.cpp`
- Modify: `test/test_material.cpp`

- [ ] **Step 1: `material.h` 재작성**

`#include` 아래에 전방 선언을 추가하고, 핸들 타입·시그니처·getter·`Clone` 을 교체한다.
`namespace SJH` 여는 줄 바로 다음에:

```cpp
namespace SJH
{
    class Texture;   // 비소유 관찰자 — full include 불요
    CLASS_PTR(Material);
```

`SetResolvedTextures` 를 교체:

```cpp
        /// @brief 이름으로부터 *해석된* 텍스처 관찰자 + sampler 유닛 설정.
        /// @details 텍스처 소유자(ResourceRegistry 또는 Model)가 Material 보다 오래 산다는 불변식 전제.
        void SetResolvedTextures(const Texture *diffuse, GLint diffuseUnit,
                                 const Texture *specular, GLint specularUnit)
        {
            mDiffuseTexture = diffuse;
            mDiffuseUnit = diffuseUnit;
            mSpecularTexture = specular;
            mSpecularUnit = specularUnit;
        }

        /// @brief 공유 템플릿을 per-use 가변 인스턴스로 복제 (Unreal MID / Unity renderer.material 패턴).
        MaterialUPtr Clone() const
        {
            return MaterialUPtr(new Material(*this));
        }
```

getter 2개와 `IsResolved` 를 교체:

```cpp
        const Texture *GetDiffuseTexture() const { return mDiffuseTexture; }
        const Texture *GetSpecularTexture() const { return mSpecularTexture; }
```
```cpp
        /// @brief 디퓨즈 텍스처가 해석된 상태인지 (nullptr 이면 미해석).
        bool IsResolved() const { return mDiffuseTexture != nullptr; }
```

이름 setter 안의 핸들 무효화를 `nullptr` 로:

```cpp
        void SetDiffuseTextureName(const std::string &name)
        {
            mDiffuseTextureName = name;
            mDiffuseTexture = nullptr;
        }
```
```cpp
        void SetSpecularTextureName(const std::string &name)
        {
            mSpecularTextureName = name;
            mSpecularTexture = nullptr;
        }
```

멤버 2개를 교체:

```cpp
        const Texture *mDiffuseTexture{nullptr};  ///< 이름으로부터 해석된 비소유 텍스처 관찰자
        const Texture *mSpecularTexture{nullptr}; ///< 스페큘러 맵 관찰자
```

파일 상단 `@note` 의 stale 문구("현재 `SJH::` 네임스페이스 *외부* 에 정의 — ... 향후 `SJH::` 로 이동 예정")를 삭제한다 — 실제로는 이미 `namespace SJH` 안에 있다.

- [ ] **Step 2: `resource_registry.cpp` `CreateMaterial` 의 `SetResolvedTextures` 호출 수정**

`CreateMaterial` 안에서 `const GLuint diffuseHandle = ...` 줄을 제거하고, 텍스처 emplace 후의
`SetResolvedTextures` 호출을 관찰자 기반으로 교체:

```cpp
        auto [texIt, texOk] = textures.emplace(key, std::move(texture));
        const Texture *diffusePtr = texIt->second.get();

        auto material = Material::Create();
        material->SetDiffuseTextureName(key);
        material->SetResolvedTextures(diffusePtr, /*diffuseUnit*/ 0,
                                      /*specular*/ nullptr, /*specularUnit*/ 1);
```

- [ ] **Step 3: `model.cpp` 의 `SetResolvedTextures` 호출 수정**

`LoadByAssimp` 안 `glMaterial->SetResolvedTextures(...)` 호출을 교체 (`diffuse`/`specular` 는 현재
`TexturePtr` — `.get()` 으로 관찰자 추출):

```cpp
            glMaterial->SetResolvedTextures(
                /*diffuse */ diffuse ? diffuse.get() : nullptr,
                /*diffuseUnit*/ 0,
                /*specular*/ specular ? specular.get() : nullptr,
                /*specularUnit*/ 1);
```

- [ ] **Step 4: `context.cpp` 의 Material 사용처 수정**

`Init()` 의 `SetResolvedTextures` 호출(Task 2 Step 3에서 작성한 부분)을 관찰자 기반으로 교체:

```cpp
        mMaterial = Material::Create();
        mMaterial->SetTextureNames("white", "gray");
        mMaterial->SetResolvedTextures(
            mRM->FindTexture("white"), 0,
            mRM->FindTexture("gray"), 1);
```

`Render()` 의 텍스처 바인딩 블록(현재 266~269행)을 관찰자 deref + null 가드로 교체:

```cpp
            // Material 의 해석된 관찰자를 자기 유닛에 바인딩.
            if (const Texture *dt = mMaterial->GetDiffuseTexture())
            {
                glActiveTexture(GL_TEXTURE0 + mMaterial->GetDiffuseUnit());
                glBindTexture(GL_TEXTURE_2D, dt->GetTextureID());
            }
            if (const Texture *st = mMaterial->GetSpecularTexture())
            {
                glActiveTexture(GL_TEXTURE0 + mMaterial->GetSpecularUnit());
                glBindTexture(GL_TEXTURE_2D, st->GetTextureID());
            }
```

- [ ] **Step 5: `test_material.cpp` 수정**

`Material` 은 `const Texture*` 를 *저장/반환만* 하고 절대 dereference 하지 않으므로,
테스트는 GL 컨텍스트 없이 *센티넬 포인터*로 검증한다 (실제 Texture 객체 불필요).

파일 상단 doxygen 의 `@note 본 클래스는 *namespace SJH 미적용*...` 문단을 삭제한다.

`#include "shader/material.h"` 아래에 추가:

```cpp
// Material 은 Texture* 를 저장/반환만 하고 deref 하지 않음 — 센티넬 포인터로 충분 (GL 불요).
static const SJH::Texture* const kDiffuseA = reinterpret_cast<const SJH::Texture*>(0xD1FFA);
static const SJH::Texture* const kSpecB    = reinterpret_cast<const SJH::Texture*>(0x5EC8B);
```

`"Material default 생성"` TEST_CASE 의 핸들 단언 2줄을 교체:

```cpp
    REQUIRE(m.GetDiffuseTexture()  == nullptr);
    REQUIRE(m.GetSpecularTexture() == nullptr);
```

`"Material::SetTextureNames"` TEST_CASE 의 핸들 단언 2줄을 동일하게 `== nullptr` 로 교체.

`"Material — 이름 변경 시 해석된 핸들 *무효화*"` TEST_CASE 를 교체:

```cpp
TEST_CASE("Material — 이름 변경 시 해석된 핸들 *무효화*", "[material][names][invalidation]")
{
    auto m_uptr = SJH::Material::Create();
    SJH::Material &m = *m_uptr;
    m.SetTextureNames("container2", "container2_specular");
    m.SetResolvedTextures(kDiffuseA, /*diffUnit*/ 2, kSpecB, /*specUnit*/ 3);

    REQUIRE(m.IsResolved());
    REQUIRE(m.GetDiffuseTexture()  == kDiffuseA);
    REQUIRE(m.GetSpecularTexture() == kSpecB);

    m.SetDiffuseTextureName("wood");
    REQUIRE(m.GetDiffuseTextureName() == "wood");
    REQUIRE(m.GetDiffuseTexture()  == nullptr);   // 무효화됨
    REQUIRE(m.GetSpecularTexture() == kSpecB);    // 영향 없음
    REQUIRE_FALSE(m.IsResolved());

    m.SetSpecularTextureName("metal");
    REQUIRE(m.GetSpecularTexture() == nullptr);
}
```

`"Material::SetResolvedTextures — 핸들 + 유닛 동시 설정"` TEST_CASE 를 교체:

```cpp
TEST_CASE("Material::SetResolvedTextures — 관찰자 + 유닛 동시 설정", "[material][resolve]")
{
    auto m_uptr = SJH::Material::Create();
    SJH::Material &m = *m_uptr;
    m.SetResolvedTextures(kDiffuseA, /*diffUnit*/ 5, kSpecB, /*specUnit*/ 6);

    REQUIRE(m.GetDiffuseTexture()  == kDiffuseA);
    REQUIRE(m.GetSpecularTexture() == kSpecB);
    REQUIRE(m.GetDiffuseUnit()  == 5);
    REQUIRE(m.GetSpecularUnit() == 6);
    REQUIRE(m.IsResolved());
}
```

`"Material — 복사 시멘틱"` TEST_CASE 안 `a.SetResolvedTextures(7, 1, 8, 2);` 를
`a.SetResolvedTextures(kDiffuseA, 1, kSpecB, 2);` 로, 핸들 단언 2줄을
`REQUIRE(b.GetDiffuseTexture() == kDiffuseA);` / `REQUIRE(b.GetSpecularTexture() == kSpecB);` 로 교체.

- [ ] **Step 6: `Clone()` 신규 테스트 추가**

`test_material.cpp` 끝에 추가:

```cpp
TEST_CASE("Material::Clone — 독립 인스턴스, 원본 불변", "[material][clone]")
{
    auto base = SJH::Material::Create();
    base->SetTextureNames("brick", "brick_spec");
    base->SetResolvedTextures(kDiffuseA, 0, kSpecB, 1);
    base->SetShininess(64.0f);

    auto variant = base->Clone();   // MaterialUPtr
    REQUIRE(variant != nullptr);
    REQUIRE(variant.get() != base.get());                          // 다른 인스턴스
    REQUIRE(variant->GetDiffuseTextureName() == "brick");          // 값 복제
    REQUIRE(variant->GetDiffuseTexture() == kDiffuseA);
    REQUIRE_THAT(variant->GetShininess(), WithinAbs(64.0f, 1e-6f));

    // variant 의 텍스처 교체가 base 에 영향 없음.
    variant->SetDiffuseTextureName("brick_wet");
    REQUIRE(variant->GetDiffuseTextureName() == "brick_wet");
    REQUIRE(variant->GetDiffuseTexture() == nullptr);              // variant 핸들 무효화
    REQUIRE(base->GetDiffuseTextureName() == "brick");             // base 불변
    REQUIRE(base->GetDiffuseTexture() == kDiffuseA);
}
```

- [ ] **Step 7: 빌드 + 테스트**

Run:
```bash
cmake --build build_Darwin
ctest --test-dir build_Darwin --output-on-failure
```
Expected: 빌드 성공, 모든 테스트 통과 (`[material][clone]` 포함).

- [ ] **Step 8: 커밋**

```bash
git add -A
git commit -m "[refactor] : Material 이 GLuint 대신 const Texture* 관찰자 보유

소유 의도를 타입에 노출 — Unreal UTexture* / Unity 텍스처 ref 와 정렬.
Clone() 추가 — 공유 템플릿을 per-use 인스턴스로 복제 (동일 마테리얼 다른 텍스쳐).

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
```

---

## Task 4: `shared_ptr` 제거 — `Model` / `RenderUnit` / `Context::mMaterial`

소유 슬롯은 `UPtr`, 참조 슬롯은 raw `T*` 관찰자. `shared_ptr` 폐기.

**Files:**
- Modify: `src/object/model.h`, `src/object/model.cpp`
- Modify: `src/context/context.h`

- [ ] **Step 1: `model.h` 의 `RenderUnit` 과 멤버 타입 교체**

```cpp
    struct RenderUnit {
        MeshUPtr  mesh;       ///< 1 RenderUnit : 1 Mesh — RenderUnit 이 유일 소유
        Material* material;   ///< 비소유 관찰자 — owner 는 Model::mMaterials
    };
```

`GetMesh` 반환 타입을 `Mesh*` 로:

```cpp
        Mesh *GetMesh(int index) const { return mRenderUnit[index].mesh.get(); }
```

멤버 벡터 2개 타입 교체:

```cpp
        std::vector<RenderUnit>    mRenderUnit;   ///< 메시 목록 (Material 관찰자 보유).
        std::vector<MaterialUPtr>  mMaterials;    ///< Material 인스턴스 — 인덱스는 assimp mMaterialIndex.
        std::vector<TextureUPtr>   mTextures;     ///< 로드한 텍스처 — Material 핸들의 lifetime owner.
```

- [ ] **Step 2: `model.cpp` 의 `LoadTexture` 람다와 push 로직 교체**

`LoadTexture` 람다를 `Texture*`(관찰자) 반환으로 교체 — UPtr 은 `mTextures` 가 소유:

```cpp
        auto LoadTexture = [&](aiMaterial *material, aiTextureType type) -> Texture*
        {
            if (material->GetTextureCount(type) <= 0)
                return nullptr;
            aiString filepath;
            material->GetTexture(type, 0, &filepath);
            const auto fullPath = fmt::format("{}/{}", dirname, filepath.C_Str());
            auto image = Image::Load(filepath.C_Str(), fullPath);
            if (!image)
                return nullptr;
            auto tex = Texture::CreateTexture(image.get());   // TextureUPtr
            if (!tex)
                return nullptr;
            mTextures.push_back(std::move(tex));              // Model 이 lifetime owner
            return mTextures.back().get();                    // 비소유 관찰자 반환
        };
```

`diffuse` / `specular` 지역 변수 타입이 이제 `Texture*` 이므로 `SetResolvedTextures` 호출의
`.get()` 을 제거 (Task 3 Step 3에서 넣었던 것):

```cpp
            const auto diffuse  = LoadTexture(aiMat, aiTextureType_DIFFUSE);
            const auto specular = LoadTexture(aiMat, aiTextureType_SPECULAR);

            glMaterial->SetResolvedTextures(
                /*diffuse */ diffuse,  /*diffuseUnit*/ 0,
                /*specular*/ specular, /*specularUnit*/ 1);

            mMaterials.push_back(std::move(glMaterial));
```

`ProcessMesh` 의 material 참조와 push 를 관찰자 기반으로 교체:

```cpp
        auto glMesh = Mesh::Create(vertices, indices, GL_TRIANGLES);
        Material* mat = nullptr;
        if (mesh->mMaterialIndex < mMaterials.size())
            mat = mMaterials[mesh->mMaterialIndex].get();
        mRenderUnit.push_back({std::move(glMesh), mat});
```

- [ ] **Step 3: `context.h` 의 `mMaterial` 타입 교체**

```cpp
        /// @brief 표면 머티리얼. Material::Create() 로 Init 에서 초기화.
        MaterialUPtr mMaterial;
```

`Material::Create()` 는 `MaterialUPtr` 를 반환하므로 `context.cpp` 의 `mMaterial = Material::Create();`
대입은 그대로 컴파일된다 (별도 수정 불요).

- [ ] **Step 4: 빌드 + 테스트**

Run:
```bash
cmake --build build_Darwin
ctest --test-dir build_Darwin --output-on-failure
```
Expected: 빌드 성공, 모든 테스트 통과.

- [ ] **Step 5: 커밋**

```bash
git add -A
git commit -m "[refactor] : Model/RenderUnit/Context 의 shared_ptr 제거

소유 슬롯은 UPtr, 참조 슬롯은 raw T* 관찰자로 통일.
RenderUnit.mesh=MeshUPtr(소유), RenderUnit.material=Material*(관찰).
Model::mMaterials/mTextures 를 UPtr 벡터로.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
```

---

## Task 5: Model 전역 캐시 — `CreateModel` / `FindModel`

`models` 맵을 활성화한다.

**Files:**
- Modify: `src/resource_registry/resource_registry.h`, `src/resource_registry/resource_registry.cpp`
- Modify: `test/test_texture.cpp`

- [ ] **Step 1: `resource_registry.h` 에 Model API 선언 추가**

`FindMaterial` 선언 다음에 추가:

```cpp
        /// @brief 파일에서 Model 을 *로드*해 @p key 로 캐시. 이미 있으면 실패(nullptr).
        Model *CreateModel(const std::string &key, const std::string &filename);

        /// @brief @p key 로 캐시된 Model *조회* (생성 안 함). 없으면 nullptr.
        Model *FindModel(const std::string &key);
```

`#include "object/model.h"` 가 이미 있는지 확인 — `Model` / `ModelUPtr` 에 필요 (현재 존재함, 유지).

- [ ] **Step 2: `resource_registry.cpp` 에 정의 추가**

`FindMaterial` 정의 다음에 추가:

```cpp
    Model *ResourceRegistry::CreateModel(const std::string &key, const std::string &filename)
    {
        if (models.find(key) != models.end())
        {
            spdlog::warn("CreateModel: 키 '{}' 가 이미 존재 — Find 를 먼저 호출하라", key);
            return nullptr;
        }
        auto model = Model::Load(filename);
        if (model == nullptr)
            return nullptr;
        auto [insertedIt, ok] = models.emplace(key, std::move(model));
        return insertedIt->second.get();
    }

    Model *ResourceRegistry::FindModel(const std::string &key)
    {
        auto it = models.find(key);
        return (it != models.end()) ? it->second.get() : nullptr;
    }
```

- [ ] **Step 3: `test_texture.cpp` 에 Model API 테스트 추가**

`Clear` 테스트 케이스 다음에 추가 (모델 에셋 없이 API 계약만 검증):

```cpp
TEST_CASE("ResourceRegistry::FindModel — 미생성은 미스(nullptr)", "[rm][model]")
{
    auto rm = SJH::ResourceRegistry::Create();
    REQUIRE(rm != nullptr);
    REQUIRE(rm->FindModel("any_key") == nullptr);
}

TEST_CASE("ResourceRegistry::CreateModel — 미존재 파일은 nullptr", "[rm][model]")
{
    SJH::test::GLContextFixture ctx;   // assimp 로드 경로가 GL 텍스처 생성까지 진행
    auto rm = SJH::ResourceRegistry::Create();
    REQUIRE(rm != nullptr);
    REQUIRE(rm->CreateModel("missing", "./__no_such_model__.obj") == nullptr);
    REQUIRE(rm->FindModel("missing") == nullptr);   // 실패 시 캐시에 안 들어감
}
```

- [ ] **Step 4: 빌드 + 테스트**

Run:
```bash
cmake --build build_Darwin
ctest --test-dir build_Darwin --output-on-failure
```
Expected: 빌드 성공, 모든 테스트 통과 (`[rm][model]` 포함).

- [ ] **Step 5: 커밋**

```bash
git add -A
git commit -m "[feat] : ResourceRegistry 에 Model 전역 캐시 (CreateModel/FindModel)

models 맵 활성화 — Model 자체는 전역 캐시 가능, Model 구성 자원은 Model 소유.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
```

---

## Task 6: 문서 갱신

**Files:**
- Modify: `.claude/MEMORY.md`, `.claude/architecture.md`
- Modify: `doc/pages/00-mainpage.md` (doxygen-class-graph skill 경유)

- [ ] **Step 1: `.claude/MEMORY.md` 모듈 인벤토리 갱신**

`src/resource_management/` 줄을 `src/resource_registry/` → `SJH::resource_registry` 로, 설명을
`ResourceRegistry` 클래스 + `Create`/`Find` 동사 분리 + Model 캐시 반영하여 수정.

- [ ] **Step 2: `.claude/architecture.md` 갱신**

§5 모듈 인벤토리 표의 `SJH::resource_management` 행 → `SJH::resource_registry`, 클래스명/파일명 갱신.
§11.2 lifetime ownership 절을 본 설계의 단일 불변식("ResourceRegistry 가 모든 Material/Model 보다
오래 산다")으로 갱신 — dangling handle 이 구조적으로 해소됨을 반영.

- [ ] **Step 3: doxygen 클래스 그래프 갱신**

`doxygen-class-graph` skill 을 호출해 `doc/pages/00-mainpage.md` 의 클래스 의존 그래프를
`ResourceRegistry` / `const Texture*` 관찰자 / `RenderUnit` 변경에 맞춰 갱신한다.

- [ ] **Step 4: 커밋**

```bash
git add -A
git commit -m "[doc] : ResourceRegistry 리팩토링 — MEMORY/architecture/doxygen 그래프 갱신

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
```

---

## Self-Review 결과

- **Spec 커버리지:** 설계 §3(모듈 리네임)=Task1, §4(API)=Task2, §5(Material)=Task3, §6(shared_ptr)=Task4,
  §4 Model 캐시=Task5, §8(문서)=Task6. §7 불변식은 Task3·4 의 소유 구조로 구현, §9 Non-Goals(Buffer)는 의도적 제외. 누락 없음.
- **Placeholder:** 없음 — 모든 변경 코드 블록은 실제 코드.
- **타입 일관성:** `CreateTexture(key, const Image*)`, `FindTexture(key)`, `CreateMaterial`/`FindMaterial`,
  `CreateModel`/`FindModel`, `SetResolvedTextures(const Texture*, GLint, const Texture*, GLint)`,
  `GetDiffuseTexture()→const Texture*`, `Clone()→MaterialUPtr`, `RenderUnit{MeshUPtr, Material*}` —
  Task 간 시그니처 일치 확인 완료.
