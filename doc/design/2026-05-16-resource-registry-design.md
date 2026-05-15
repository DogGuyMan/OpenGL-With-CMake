# 리소스 소유권 명확화 & ResourceRegistry 리팩토링 — 설계

- **날짜**: 2026-05-16
- **대상 모듈**: `src/resource_management/` (→ `src/resource_registry/`), `src/object/`, `src/shader/`, `src/context/`
- **상태**: 승인됨 (구현 계획 수립 단계)

## 1. 동기 & 목표

`ResourceManagement`는 `Texture`/`Image`/`Material`을 이름 키로 캐시하는 매니저였으나,
리소스의 *소유 정책*이 이름·타입·호출지점 어디에도 드러나지 않았다. 그 결과:

- `Load*` 동사 하나가 "디스크 읽기 / get-or-create / 순수 lookup" 세 가지를 의미.
- `TexturePtr` 하나만 봐서는 전역 캐시 소유인지 Model 소유인지 알 수 없음 — `Material`의
  raw `GLuint` dangling handle 위험(architecture.md §11.2)의 근본 원인.
- `models` 맵은 선언만 되고 미구현 — `Clear()`도 누락.

**목표**: 리소스의 3가지 의도가 코드 스스로 드러나도록 재정비한다.

| 의도 | 소유자 | 수명 |
|---|---|---|
| 1. 전역 — 자주 쓰이고 공유되는 자원 | `ResourceRegistry` | Registry(세션) 수명 |
| 2. 스코프 한정 — 만들고 버리는 일회성 자원 | 호출 스코프 지역 `UPtr` | 스코프 종료 시 |
| 3. Model 구성 — Model에 강결합된 자원 | `Model` 멤버 `UPtr` | Model과 함께 |

## 2. 네이밍 컨벤션 (3 규칙)

### 컨벤션 1 — 생성 진입점이 소유를 선언한다

| 의도 | 생성 호출 | 반환 | 소유자 |
|---|---|---|---|
| 2. 스코프 한정 | `Image::Load(...)`, `Mesh::Create(...)` — 타입 자신의 팩토리 직접 호출 | `XxxUPtr` 지역변수 | 호출 스코프 |
| 3. Model 구성 | Model 내부에서 타입 팩토리 호출 → `mXxx` 멤버로 `std::move` | Model 멤버 `XxxUPtr` | Model |
| 1. 전역 | `ResourceRegistry::CreateXxx(key, ...)` | 비소유 `Xxx*` | Registry |

`Type::Create` 직접 호출 = 비전역. `Registry::Create` 경유 = 전역.

### 컨벤션 2 — 타입이 소유를 말한다

- `XxxUPtr` 손에 쥠 = "내가(또는 내가 `move`할 대상이) 소유. 전역 아님."
- raw `Xxx*` 손에 쥠 = "비소유 관찰자. Registry 또는 Model이 소유 중."
- `shared_ptr`(`XxxPtr`)은 본 리팩토링 범위에서 **전면 폐기**. 소유 슬롯은 `UPtr`, 참조 슬롯은 raw `T*`.

### 컨벤션 3 — `Create` vs `Find` 엄격 분리

- `CreateXxx` — 항상 새로 생성. Registry에서 키가 이미 있으면 **실패**(`spdlog::warn` + `nullptr`),
  기존 인스턴스를 슬쩍 반환하지 않는다.
- `FindXxx` — 순수 조회. 없으면 `nullptr`. **절대 생성하지 않는다.**
- 암시적 get-or-create 금지. 필요하면 호출자가 `Find` → null이면 `Create`, 두 줄로 명시.

## 3. 모듈 리네임 (전부 통일)

| 현재 | 변경 후 |
|---|---|
| 디렉터리 `src/resource_management/` | `src/resource_registry/` |
| CMake 타겟 `sjhopengl_resource_management` | `sjhopengl_resource_registry` |
| alias `SJH::resource_management` | `SJH::resource_registry` |
| 파일 `resource_management.{h,cpp}` | `resource_registry.{h,cpp}` |
| 헤더 가드 `__SJH_RESOURCE_MANAGEMENT_H__` | `__SJH_RESOURCE_REGISTRY_H__` |
| 클래스 `ResourceManagement` | `ResourceRegistry` |
| 팩토리 `CreateRM()` | `Create()` |
| `CLASS_PTR(ResourceManagement)` | `CLASS_PTR(ResourceRegistry)` |
| `#include "resource_management/..."` (model.h, context 등) | `"resource_registry/..."` |
| `friend class ResourceManagement` (texture.h:33) | **제거** |

- `image.{h,cpp}`, `texture.{h,cpp}` 파일명은 유지.
- `friend` 제거 근거: `ResourceRegistry`는 public `Texture::CreateTexture`만 사용 — friend가 불필요
  (architecture.md §11.1 `Program`/`Uniforms` "friend 8개 → 0개" 진화와 같은 교훈).
- `src/CMakeLists.txt`의 `add_subdirectory(resource_management)` → `resource_registry`.

## 4. `ResourceRegistry` API

| 현재 | 변경 후 | 비고 |
|---|---|---|
| `LoadTextureFromImage(image)` | `CreateTexture(key, image)` | 생성 + 캐시 |
| `LoadTextureWithName(name)` | `FindTexture(key)` | "Load인데 lookup"인 거짓 이름 해소 |
| `LoadMaterial(name, path)` | `CreateMaterial(key, path)` + `FindMaterial(key)` | |
| (미구현) | `CreateModel(key, filename)` + `FindModel(key)` | `models` 맵 활성화 |
| `LoadImage` / `images` 맵 | **삭제** | Image = 스코프 한정(컨벤션 1) |
| `Clear()` | `models.clear()` 추가 | 현재 누락 버그 수정 |

- 캐시 맵은 전부 `UPtr` 저장: `unordered_map<string, TextureUPtr/MaterialUPtr/ModelUPtr>`.
- 외부에는 비소유 `T*` 반환.
- Image는 더 이상 캐시하지 않는다. `CreateTexture`가 Texture를 만들 때 필요한 Image는
  메서드 내부 지역 `ImageUPtr`로 생성·소비·소멸 — 컨벤션 1의 스코프 한정 그대로.

## 5. `Material` — `Texture*` 관찰자 + `Clone()`

- `GLuint mDiffuseTexture` / `GLuint mSpecularTexture` → **`const Texture*`** 비소유 관찰자.
  - material.h에 `namespace SJH { class Texture; }` 전방 선언 추가 (full include 불요).
- `SetResolvedTextures(...)` 시그니처: `GLuint` 핸들 인자 → `const Texture*`.
- `GetDiffuseTexture()` / `GetSpecularTexture()` 반환 타입 → `const Texture*`.
  bind 시점에 호출자가 `->GetTextureID()`.
- `IsResolved()` — `mDiffuseTexture != nullptr` 로 갱신.
- **`Clone() → MaterialUPtr` 추가** — 공유 템플릿(Registry 소유)을 가벼운 per-use 인스턴스로 복제.
  "동일 마테리얼, 다른 텍스쳐"의 정석 (Unreal `UMaterialInstanceDynamic` / Unity `renderer.material`).
  `Material`은 작아(문자열 2 + 포인터/유닛 4 + float 1) 값 복사로 충분.
- 이름 키 슬롯 + 해석 핸들 캐시 + 이름 setter의 핸들 무효화 구조는 엔진 정렬(Unity 텍스처 레퍼런스,
  Unreal FName 텍스처 파라미터) — **유지**.
- material.h:26 stale 주석("현재 `SJH::` 네임스페이스 *외부*에 정의") 수정 — 실제로는 `SJH` 안.

## 6. `Model` / `RenderUnit` — `shared_ptr` 제거

```cpp
struct RenderUnit {
    MeshUPtr  mesh;       // 1 RenderUnit : 1 Mesh — RenderUnit이 유일 소유
    Material* material;   // 비소유 관찰자 — owner는 Model::mMaterials
};

class Model {
    std::vector<RenderUnit>   mRenderUnit;
    std::vector<MaterialUPtr> mMaterials;  // MaterialPtr → MaterialUPtr
    std::vector<TextureUPtr>  mTextures;   // TexturePtr  → TextureUPtr
};
```

- `GetMesh(int)` 반환 `MeshPtr` → `Mesh*` (비소유 관찰자).
- `Model::LoadByAssimp`의 `LoadTexture` 람다: `Texture::CreateTexture`(`TextureUPtr`)를 `mTextures`로
  `std::move`. `SetResolvedTextures`에는 `mTextures.back().get()`(raw 관찰자) 전달.
- `RenderUnit.material`은 `mMaterials[idx].get()` — 여러 mesh가 한 material을 공유하므로
  소유는 `mMaterials`(`UPtr`) 단일, RenderUnit은 관찰만.

## 7. Lifetime Invariant (dangling 방지)

> **단일 불변식: `ResourceRegistry`는 모든 `Material`·`Model`보다 오래 산다.**
> Registry는 세션(Context) 수명을 가지며 per-item evict를 하지 않는다 (`Clear()`는 일괄 종료용).

이 하나로 §11.2 dangling handle이 구조적으로 해소된다:

- Registry 텍스처를 참조하는 어떤 `Material`(템플릿/클론/Model 내부)도 Registry 생존 동안 안전.
- Model 내부 자원(`mTextures`/`mMaterials`)은 Model과 함께 cascade 소멸 — Model 내부에서만 참조되므로 안전.
- ref-count(`shared_ptr` / Cocos `Ref`) 없이 이 불변식만으로 등가 안전 확보.

## 8. 소비자 / 테스트 / 문서 갱신

- `src/context/context.{h,cpp}` — 리네임된 모듈 경로·클래스·API·타입으로 호출지점 갱신.
- `test/test_material.cpp` 외 영향 테스트 — 변경된 `ResourceRegistry`/`Material` API 반영.
- `.claude/MEMORY.md` 모듈 인벤토리 표 갱신.
- `architecture.md` §5(모듈 인벤토리), §11.2(lifetime ownership) 갱신.
- doxygen 클래스 의존 그래프(`doc/pages/00-mainpage.md`) 갱신 — `doxygen-class-graph` skill 사용.

## 9. 범위 밖 (Non-Goals)

- `Buffer` / `BufferPtr` (Mesh 내부 정점/인덱스 버퍼)의 `shared_ptr` 제거 — 별도 후속 Phase.
- `ResourceRegistry`의 per-item evict / LRU 등 캐시 정책 — 현재 무-evict 유지.
- Model 임포트 텍스처를 Registry로 흡수하는 통합 — Model 소유 유지(의도 3).

## 10. 검토했으나 채택하지 않은 대안

| 대안 | 기각 사유 |
|---|---|
| `Material`이 `TextureUPtr` 직접 소유 | 자기완결적이나 텍스처 공유 불가. 엔진 3종 모두 Texture를 별도 시스템이 소유 |
| Unity `MaterialPropertyBlock` 식 무복사 오버라이드 | 간접층 과함. `Material`이 작아 `Clone()`이 더 단순 (YAGNI) |
| ref-count (`shared_ptr` / Cocos `Ref`) | 사용자 결정으로 폐기. §7 단일 불변식으로 등가 안전 확보 |
| 모듈/클래스만 리네임, API 동사 유지 | `Load*` 3중 의미 모호성이 남음 — 컨벤션 3로 해소 필요 |

## 11. 엔진 조사 근거 (Context7)

| 엔진 | Texture 소유자 | Material→Texture 참조 | 런타임 텍스처 교체 |
|---|---|---|---|
| Unreal | `UObject` GC (전역) | FName 키 텍스처 파라미터 | `UMaterialInstanceDynamic::SetTextureParameterValue` |
| Unity | 에셋 시스템 (전역) | 문자열 키 텍스처 레퍼런스 | `MaterialPropertyBlock` / `renderer.material` 인스턴스 |
| Cocos2d-x | `TextureCache` 싱글톤 + `Ref` 카운팅 | 캐시 이름/경로 키 조회 | Sprite 텍스처 교체, 캐시 유지 |

공통 결론: ① Texture는 Material이 소유하지 않는다, ② 텍스처 슬롯은 이름/키 기반,
③ "동일 마테리얼 다른 텍스쳐"는 인스턴스 패턴, ④ 수명은 전역 소유자 + (ref-count|GC).
본 설계는 ④의 ref-count를 §7 단일 불변식으로 치환.

## 12. 구현 Phase 분해 (구현 계획서에서 상세화)

빌드가 매 단계 green을 유지하도록:

1. **모듈/클래스 리네임** — `resource_management` → `resource_registry`, `ResourceManagement` →
   `ResourceRegistry`, `friend` 제거. 동작 변화 없음.
2. **API 동사 정리** — `Create`/`Find` 분리, `images` 맵 제거, `Clear()` 수정.
3. **Material 소유권** — `GLuint` → `const Texture*` 관찰자, `Clone()` 추가, `SetResolvedTextures`
   시그니처 변경.
4. **`shared_ptr` 제거** — `RenderUnit`/`Model`을 `UPtr` + 관찰자로 통일.
5. **Model 전역 캐시** — `CreateModel`/`FindModel` 구현.
6. **소비자·테스트·문서 갱신** — Context, 테스트, MEMORY.md, architecture.md, doxygen 그래프.
