# Transform 강화 & intrusive Scene Graph — 설계

- **날짜**: 2026-05-19
- **대상 모듈**: `src/object/` (transform / scene_node / scene_graph), `src/context/`
- **상태**: 승인됨 (구현 계획 수립 단계)
- **설계 출처**: 본 문서. Context7 엔진 API 조사 2회 (Transform / Scene Graph).

## 1. 동기 & 목표

`SJH::Transform`(`src/object/transform.h`)은 TRS + 부모-자식 계층 + `GetModelMatrix()`를
갖췄으나 다음 문제가 있었다:

- **계층 불변식 미보장**: `Parent`·`Children`가 둘 다 public 인데 동기화 메서드가 없어,
  `Parent`를 바꾸면 옛 부모의 `Children` 맵이 stale 해진다 (구조적 버그).
- **캐시 부재**: `GetModelMatrix()`가 호출마다 조상 체인 전체를 재귀 재계산 (O(depth)/호출).
- **미사용**: `Context::Render`는 `Transform`을 쓰지 않고 매 드로우 사이트마다
  `glm::translate * glm::rotate * glm::scale`을 inline 으로 직접 만든다 (8곳).

**목표**: 엔진 모범 설계(intrusive 노드 트리 + 월드 행렬 캐싱)를 흡수해 Transform 계층을
강화하고, `Context`가 실제로 이를 사용하도록 연결한다.

### 범위 결정 (브레인스토밍 합의)

| 항목 | 결정 |
|---|---|
| 강화 범위 | Moderate + Context 적용 — 캡슐화 setter + 캐싱 + 월드 접근 + Context 실사용 |
| 회전 표현 | Euler 유지 (vec3, degree) — Camera.h `EulerYaw/Pitch` 컨벤션과 일관. 짐벌락은 문서 경고 |
| 캐싱 전략 | B — 월드 행렬 캐시 + 하향 dirty 전파 |
| 씬 그래프 토폴로지 | intrusive 노드 트리 (엔진 만장일치 모범) |
| 노드 인덱싱 | 정수값 enum (`SceneNodeId`) = `std::array` 인덱스 |
| `INameTagInterface` | 제거 (enum 이 노드 정체성을 대체) |

## 2. 조사 근거 (Context7)

### 2.1 Transform API — 4개 엔진 공통

Unity / Unreal / Godot / Cocos2d-x 가 수렴하는 모범 패턴:

1. **로컬/월드 이원성** — 로컬 TRS 저장, 월드는 부모 합성으로 노출.
2. **Dirty-flag 행렬 캐싱** — 월드 행렬을 접근마다 재계산하지 않고 캐시 + 변경 시 무효화.
3. **불변식을 지키는 캡슐화 setter** — `SetParent`/`AttachTo`가 부모·자식을 동시 일관 유지.
4. (Quaternion 회전 — 본 프로젝트는 학습 목적상 Euler 유지, 채택하지 않음.)

### 2.2 Scene Graph — 4개 엔진 공통

| 엔진 | 단위 | 정수/키 인덱싱 |
|---|---|---|
| Godot | `Node` 포인터 트리 | `NodePath` 문자열 경로 |
| Unity | `Transform` 자체가 그래프 | `GetChild(int siblingIndex)` |
| Cocos2d-x | `Node` 포인터 트리 | **`getChildByTag(int)`** — 정수 tag 조회 |
| Unreal | `USceneComponent` 부착 트리 | — |

**핵심 발견**: 4개 엔진 전부 **intrusive 노드 트리** — 각 노드가 자기 `parent`·`children`를
직접 보유한다. 토폴로지를 별도 배열로 분리하는 external/DOD 방식은 어느 엔진도 씬 그래프에
쓰지 않는다 (Unity DOTS 같은 데이터 지향 *시스템* 의 방식). 정수 인덱싱은 Cocos2d-x
`getChildByTag(int)` 가 선례 — intrusive 트리에 정수 tag 를 붙여 O(1) 조회.

➡️ 본 설계는 **intrusive 노드 트리 + enum 정수 인덱싱**을 채택한다.

## 3. `Transform` 변경 — 순수 값 객체로 축소

`Transform`은 계층 책임을 `SceneNode`로 넘기고 *순수 로컬 TRS 값 객체*가 된다.

```cpp
namespace SJH
{
    /// @brief 로컬 TRS 값 객체. 계층/월드 합성은 SceneNode 가 담당.
    class Transform
    {
    public:
        glm::vec3 Translate = glm::vec3(0.0f);          ///< 이동량
        glm::vec3 EulerRot  = glm::vec3(0.0f);          ///< 오일러 회전각 (degree, XYZ)
        glm::vec3 Scale     = glm::vec3(1.0f);          ///< 스케일 팩터

        /// @brief 로컬 모델 행렬 — T·Rz·Ry·Rx·S 순서.
        glm::mat4 GetLocalMatrix() const;
    };

    /// @brief UV 좌표계 변환 — 무변경, 독립 관심사.
    class UVTransform { /* Offset / Scale / RotationDeg — 현행 유지 */ };
}
```

- **제거**: `INameTagInterface`, `Transform::Name`, `Transform::GetName()`, `Parent`,
  `Children`, `GetModelMatrix()`.
- **제거**: `#include <glad/glad.h>` — `Transform`/`UVTransform`은 GL 타입을 쓰지 않는다
  (glm 만 필요).
- **유지**: `UVTransform` 전체.
- TRS 는 public 유지 — dirty 추적은 `SceneNode`가 setter 경유로 담당하므로 캡슐화 불요.
  Camera/Light 의 public POD 컨벤션과 일관.
- `GetLocalMatrix()`는 현 `GetModelMatrix()`의 *비-재귀* 부분과 동일 행렬을 만든다:
  `translate(Translate) · rotate(radians(EulerRot.z),Z) · rotate(radians(EulerRot.y),Y)
  · rotate(radians(EulerRot.x),X) · scale(Scale)`.

## 4. `SceneNode` — 신규 (`src/object/scene_node.{h,cpp}`)

intrusive 트리 노드. `Transform`을 payload 로 들고 계층·월드 캐시를 책임진다.

```cpp
namespace SJH
{
    class SceneNode
    {
    public:
        SceneNode() = default;
        ~SceneNode() = default;
        SceneNode(const SceneNode &) = delete;            ///< 배열 저장 + 포인터 참조 → 주소 고정
        SceneNode &operator=(const SceneNode &) = delete;
        SceneNode(SceneNode &&) = delete;
        SceneNode &operator=(SceneNode &&) = delete;

        // --- 읽기 ---
        const Transform &Local() const { return mLocal; }
        glm::mat4 World() const;                          ///< dirty 면 재계산 후 캐시
        SceneNode *Parent() const { return mParent; }
        const std::vector<SceneNode *> &Children() const { return mChildren; }

        // --- 로컬 변경 (dirty 전파 동반) ---
        void SetLocal(const Transform &local);
        void SetTranslate(const glm::vec3 &t);
        void SetEulerRot(const glm::vec3 &r);
        void SetScale(const glm::vec3 &s);

        // --- 토폴로지 (불변식 보장) ---
        void Attach(SceneNode *child);                    ///< child 를 자식으로. 순환 거부.
        void Detach(SceneNode *child);

    private:
        void MarkSubtreeDirty();                          ///< 자신 + 자손 DFS dirty
        bool IsAncestorOf(const SceneNode *node) const;   ///< 순환 가드용

        Transform mLocal;
        SceneNode *mParent = nullptr;                     ///< 비소유 — SceneGraph 가 소유
        std::vector<SceneNode *> mChildren;               ///< 비소유 관찰자
        mutable glm::mat4 mWorldCache{1.0f};
        mutable bool mDirty = true;
    };
}
```

### 4.1 `World()` — 캐싱 B

```cpp
glm::mat4 SceneNode::World() const
{
    if (mDirty)
    {
        const glm::mat4 parentWorld = mParent ? mParent->World() : glm::mat4(1.0f);
        mWorldCache = parentWorld * mLocal.GetLocalMatrix();
        mDirty = false;
    }
    return mWorldCache;
}
```

- 상향 재귀가 dirty 조상도 자연 재계산. clean 상태면 O(1) 반환.
- `mWorldCache`/`mDirty` 가 `mutable` 인 이유: `World()`는 논리적으로 const(관찰자)지만
  캐시를 갱신한다. Unity `localToWorldMatrix` getter 와 같은 관용.

### 4.2 dirty 하향 전파

`SetLocal`/`SetTranslate`/`SetEulerRot`/`SetScale`/`Attach`/`Detach`는 변경 후
`MarkSubtreeDirty()`를 호출한다 — 자신과 모든 자손을 DFS 로 `mDirty = true` 표시.
이로써 자손의 캐시가 stale 임이 보장되고, 다음 `World()` 호출이 재계산한다.

### 4.3 `Attach` / `Detach` — 불변식

```
Attach(child):
  - child == this        → 거부 (자기 자신 부착 불가)
  - child->IsAncestorOf(this) → 거부 (순환 — child 가 this 의 조상이면 사이클)
  - child->mParent != nullptr → child->mParent->Detach(child)  (기존 부모에서 분리)
  - child->mParent = this; mChildren.push_back(child)
  - child->MarkSubtreeDirty()

Detach(child):
  - mChildren 에서 child 제거
  - child->mParent = nullptr
  - child->MarkSubtreeDirty()
```

`mParent` 포인터와 `mChildren` 벡터는 항상 동시에 갱신된다 — 현 `Transform`의 stale 버그
해소. 순환 가드는 `IsAncestorOf`(this→root 상향 탐색)로 구현.

## 5. `SceneGraph<TId>` — 신규 (`src/object/scene_graph.h`, 템플릿 헤더 온리)

노드 소유자 + enum→노드 O(1) 조회 파사드.

```cpp
namespace SJH
{
    /// @brief enum 인덱스 기반 씬 그래프. TId 는 Count 멤버를 가진 enum 가정.
    template <typename TId>
    class SceneGraph
    {
    public:
        static constexpr std::size_t Count = static_cast<std::size_t>(TId::Count);

        SceneNode       &At(TId id)       { return mNodes[Index(id)]; }
        const SceneNode &At(TId id) const { return mNodes[Index(id)]; }

        glm::mat4 World(TId id) const     { return mNodes[Index(id)].World(); }
        void SetLocal(TId id, const Transform &local) { mNodes[Index(id)].SetLocal(local); }

        void Attach(TId child, TId parent) { mNodes[Index(parent)].Attach(&mNodes[Index(child)]); }
        void Detach(TId child);            ///< child 를 현재 부모에서 분리

    private:
        static std::size_t Index(TId id)  { return static_cast<std::size_t>(id); }

        std::array<SceneNode, Count> mNodes;   ///< enum 정수값 = 인덱스. 주소 고정.
    };
}
```

- `std::array` 라 노드 주소가 고정 → `SceneNode*` 부모/자식 포인터가 안전.
- `SceneNode` 가 복사/이동 불가이므로 `SceneGraph` 도 복사/이동 불가 — 소유자(`Context`)는
  멤버로 in-place 생성한다 (`Context` 는 팩토리로 힙 생성되므로 문제 없음).
- 템플릿 → 헤더 온리. `object` 모듈 CMake 에 새 컴파일 단위 추가 불요 (단 `scene_node.cpp`는
  추가됨, §9 참조).
- `TId` 가 enum 이라는 점만 가정 — 단위 테스트는 자체 `enum class TestNode` 로 격리 검증.

## 6. `SceneNodeId` enum + 계층 (`src/context/scene_node_id.h`)

장면 고유 enum — `context` 모듈에 둔다 (씬 콘텐츠이므로).

```cpp
namespace SJH
{
    enum class SceneNodeId : std::size_t
    {
        Root = 0,
        Plane, Box1, Box2, Outline,
        Windows, Window0, Window1, Window2,
        PointMarker0, PointMarker1, SpotMarker,
        Count
    };
}
```

계층 구조:

```
Root
├── Plane
├── Box1
├── Box2
│   └── Outline          (Box2 의 1.05 배 — 부모 상속 데모. Box2 이동 시 자동 추종)
├── Windows              (그룹 노드)
│   ├── Window0
│   ├── Window1
│   └── Window2
├── PointMarker0         (동적 — PointLight 0 위치 추종)
├── PointMarker1         (동적 — PointLight 1 위치 추종)
└── SpotMarker           (동적 — SpotLight 위치 추종)
```

- **Outline = Box2 자식**: `World(Outline) = World(Box2) · Local(Outline)`,
  `Local(Outline).Scale = 1.05`. 현 코드 `transform * scale(1.05)` 와 수학적으로 동일.
- **Windows 그룹 노드**: 초기 그룹 로컬 = identity, `Window0/1/2` 로컬 = 현재 절대 좌표
  `(0,0.5,4)` / `(0.2,0.5,5)` / `(0.4,0.5,6)`. 그룹이 부모이므로 `Windows` 노드를
  이동하면 창 3개가 함께 이동 — 그룹화 데모는 유지된다. 시각 출력은 현행과 동일.

## 7. Context 적용

- `context.h`: 멤버 `SceneGraph<SceneNodeId> mScene;` 추가.
- `Context::Init()`: 정적 노드의 로컬 TRS 를 `mScene.SetLocal(...)` 로 설정하고
  `mScene.Attach(...)` 로 계층을 1회 구성. 현 `Render()`의 inline `modelTransform` 상수
  8곳이 여기로 이전된다.
- `Context::Render()`: inline `glm::translate*rotate*scale` 을 `mScene.World(id)` 호출로
  교체. 예) Box1 → `projMat * viewMat * mScene.World(SceneNodeId::Box1)`.
- **동적 노드**: 매 프레임 갱신 —
  `mScene.At(SceneNodeId::PointMarker0).SetTranslate(mPointLights[0].Pos)` 등.
  flashlight 모드의 SpotMarker 스킵 로직(`mFlashLightMode && i >= 2`)은 유지.
- Outline 은 Box2 자식이므로 Box2 만 갱신하면 자동 추종 — 별도 처리 불요.

## 8. 테스트

리팩토링 + 신규 동작이므로 신규 클래스에 단위 테스트를 추가한다.

### `test/test_scene_graph.cpp` (신규, GL 불요)

자체 `enum class TestNode : std::size_t { Root, A, B, C, Count }` 로 `SceneGraph<TestNode>`
검증:

- 기본 상태: 모든 노드 `World()` == identity.
- `SetLocal` → `World()` 가 로컬 행렬 반영.
- `Attach` 계층: B 를 A 아래, A 를 Root 아래 → `World(B) == World(A) * Local(B).GetLocalMatrix()`.
- **하향 전파**: A 부착 후 `SetLocal(Root)` → `World(B)` 가 갱신됨 (stale 아님).
- 캐시: 변경 없이 `World(B)` 두 번 → 동일 결과.
- 순환 가드: `Attach(Root, C)` (C 는 Root 의 자손) → 거부, 트리 불변.
- `Detach`: 분리 후 자식 `World()` 가 부모 영향 제거.

### `test/test_transform.cpp` (신규, GL 불요)

- `GetLocalMatrix` — translate 전용 / scale 전용 / Y축 90° 회전 (degree 단위 확인) /
  TRS 합성 순서.

### Context 회귀

`World(id)` 가 만드는 행렬은 기존 inline `modelTransform` 과 수학적으로 동일하므로 시각
출력은 불변이어야 한다. 골든 이미지가 있으면 그대로 통과, 없으면 육안 확인.

## 9. 영향 파일 / 문서

| 분류 | 파일 |
|---|---|
| 신규 | `src/object/scene_node.h`, `src/object/scene_node.cpp` |
| 신규 | `src/object/scene_graph.h` (템플릿 헤더 온리) |
| 신규 | `src/context/scene_node_id.h` |
| 신규 | `test/test_scene_graph.cpp`, `test/test_transform.cpp` |
| 수정 | `src/object/transform.h` — 순수 값 객체로 축소 |
| 수정 | `src/object/CMakeLists.txt` — `scene_node.cpp` 소스 추가 |
| 수정 | `src/context/context.h`, `src/context/context.cpp` — `mScene` 도입 |
| 수정 | `test/CMakeLists.txt` — `test_scene_graph` / `test_transform` 실행파일 + `tests` umbrella DEPENDS |
| 문서 | `.claude/MEMORY.md`, `.claude/architecture.md` — 모듈 인벤토리 (object 모듈에 SceneNode/SceneGraph 추가) |
| 문서 | `doc/pages/00-mainpage.md` — doxygen 클래스 의존 그래프 (`doxygen-class-graph` skill 경유) |

`src/object/CMakeLists.txt` 는 현재 `mesh.cpp model.cpp` → `scene_node.cpp` 추가.
링크 의존성은 glm 만 필요하며 `object` 모듈이 이미 `glm::glm` 을 PUBLIC 으로 가지므로
추가 의존성 없음.

## 10. 범위 밖 (Non-Goals)

- Quaternion 회전 — Euler 유지 (학습 목적, Camera 컨벤션 일관).
- `NodePath` 문자열 경로 조회 (Godot 식) — enum 인덱싱으로 충분.
- 런타임 노드 추가/삭제 — 노드 집합은 컴파일 타임 고정 (`SceneNodeId::Count`).
- 씬 그래프 순회 기반 일괄 렌더링 / `visit()` 파이프라인 — `Context::Render` 의 명시적
  드로우 순서를 유지하고 `World(id)` 만 사용. 자동 순회 렌더는 후속 과제.
- Camera / Light 를 씬 그래프 노드로 편입 — 별도 후속 Phase.

## 11. 검토했으나 채택하지 않은 대안

| 대안 | 기각 사유 |
|---|---|
| 로컬 행렬 캐시만 (캐싱 A) | depth 가 깊어지면 월드 합성이 여전히 O(depth). 사용자가 캐싱 B 선택 |
| external topology (DOD, 부모/자식/월드 배열 분리) | 어느 엔진도 씬 그래프에 안 씀. intrusive 가 모범. Transform 단독 테스트도 약화 |
| growable `std::vector<SceneNode>` 저장 | 재할당이 `SceneNode*` 무효화. 노드 집합 고정이므로 `std::array` 로 충분 |
| `Transform` 에 계층 유지 (intrusive 를 Transform 자체에) | `Transform` 이 값 객체 + 트리 노드 두 책임을 겸함. `SceneNode` 분리가 단일 책임 |
| `INameTagInterface` 문자열 이름 유지 | enum 이 노드 정체성을 대체 — 문자열 이름 중복. 제거 |

## 12. 구현 Phase 분해 (구현 계획서에서 상세화)

빌드가 매 단계 green 을 유지하도록:

1. **`Transform` 축소** — 순수 값 객체화 (`GetLocalMatrix`), `INameTagInterface`/계층 제거.
   `Transform` 소비자가 아직 없으므로(미사용) 독립적으로 안전.
2. **`SceneNode`** — intrusive 노드 + `World()` 캐싱 + `Attach`/`Detach` 불변식.
3. **`SceneGraph<TId>`** — enum 인덱스 파사드 (템플릿).
4. **테스트** — `test_transform` / `test_scene_graph`.
5. **`SceneNodeId` + Context 적용** — enum 정의, `mScene` 구성, `Render` 의 inline
   `modelTransform` 8곳 교체.
6. **문서 갱신** — MEMORY / architecture / doxygen 그래프.
