# Transform 강화 & Scene Graph Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `Transform`을 순수 값 객체로 축소하고, intrusive 노드 트리 `SceneNode` + enum 인덱싱 `SceneGraph<TId>`를 신설해 `Context`의 inline `modelTransform` 생성을 씬 그래프로 대체한다.

**Architecture:** `Transform`은 로컬 TRS 값 객체(`GetLocalMatrix()`)로만 남기고 계층 책임을 `SceneNode`로 이관한다. `SceneNode`는 부모/자식 포인터와 월드 행렬 캐시(dirty 하향 전파)를 들고, `SceneGraph<TId>`는 `std::array<SceneNode, Count>`로 모든 노드를 소유하며 enum 정수값으로 O(1) 조회한다. `Context`는 `SceneGraph<SceneNodeId>` 멤버를 1개 들고 `Render()`의 변환 8곳을 `World(id)` 호출로 교체한다.

**Tech Stack:** C++17, CMake + Make, vcpkg, Catch2 v3, glm, OpenGL/glad

**설계 출처:** [doc/design/2026-05-19-transform-scene-graph-design.md](2026-05-19-transform-scene-graph-design.md)

**공통 빌드/테스트 명령:**
```bash
# CMakeLists.txt 변경 후 — 재구성 필요 (vcpkg 스킵)
cmake --preset debug -DVCPKG_MANIFEST_INSTALL=OFF
# 코드만 수정한 경우
cmake --build build_Darwin
# 특정 테스트 실행파일 빌드 (test 는 EXCLUDE_FROM_ALL)
cmake --build build_Darwin --target test_transform
# 테스트 실행 — 실행파일은 build_Darwin/test/ 에 생성됨
./build_Darwin/test/test_transform
```

> `test/` 는 `EXCLUDE_FROM_ALL` 이라 `cmake --build build_Darwin`(=ALL) 은 테스트를 빌드하지 않는다. 테스트는 `--target <name>` 으로 명시 빌드한다.

---

## Task 1: `Transform` 순수 값 객체로 축소

`Transform`에서 계층(`Parent`/`Children`/`GetModelMatrix`)과 `INameTagInterface`를 제거하고
`GetLocalMatrix()`만 남긴다. `Transform`은 현재 어디서도 사용되지 않으므로(검증: `transform.h`를
include 하는 파일 없음) 독립적으로 안전하다.

**Files:**
- Create: `test/test_transform.cpp`
- Modify: `test/CMakeLists.txt`
- Modify: `src/object/transform.h`

- [ ] **Step 1: 실패하는 테스트 작성**

`test/test_transform.cpp` 생성:

```cpp
/**
 * @file test_transform.cpp
 * @brief SJH::Transform — GetLocalMatrix TRS 합성 검증 (GL 컨텍스트 불요).
 */
#include "object/transform.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/gtc/matrix_transform.hpp>

using Catch::Matchers::WithinAbs;

namespace
{
    // 두 mat4 가 성분별로 근사 동일한지 검사.
    void RequireMatNear(const glm::mat4 &a, const glm::mat4 &b, float eps = 1e-5f)
    {
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                REQUIRE_THAT(a[c][r], WithinAbs(b[c][r], eps));
    }
}

TEST_CASE("Transform 기본값 — identity 로컬 행렬", "[transform]")
{
    SJH::Transform t;
    RequireMatNear(t.GetLocalMatrix(), glm::mat4(1.0f));
}

TEST_CASE("Transform translate 전용", "[transform]")
{
    SJH::Transform t;
    t.Translate = glm::vec3(1.0f, 2.0f, 3.0f);
    RequireMatNear(t.GetLocalMatrix(),
                   glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f)));
}

TEST_CASE("Transform scale 전용", "[transform]")
{
    SJH::Transform t;
    t.Scale = glm::vec3(2.0f, 3.0f, 4.0f);
    RequireMatNear(t.GetLocalMatrix(),
                   glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 3.0f, 4.0f)));
}

TEST_CASE("Transform EulerRot — degree 입력이 radians 로 변환 (Y축 90도)", "[transform]")
{
    SJH::Transform t;
    t.EulerRot = glm::vec3(0.0f, 90.0f, 0.0f);
    const glm::mat4 expected =
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    RequireMatNear(t.GetLocalMatrix(), expected);
}

TEST_CASE("Transform TRS 합성 순서 — T·Rz·Ry·Rx·S", "[transform]")
{
    SJH::Transform t;
    t.Translate = glm::vec3(1.0f, 0.0f, 0.0f);
    t.EulerRot  = glm::vec3(0.0f, 0.0f, 30.0f);
    t.Scale     = glm::vec3(2.0f, 2.0f, 2.0f);
    const glm::mat4 expected =
        glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
    RequireMatNear(t.GetLocalMatrix(), expected);
}
```

- [ ] **Step 2: `test/CMakeLists.txt` 에 테스트 등록**

`test/CMakeLists.txt` 에 다음 블록을 추가한다 (다른 `add_executable(test_*)` 블록 사이, 일관된 위치):

```cmake
#  test_transform — Transform 값 객체 (GL 불요, fixture 불필요)
add_executable(test_transform test_transform.cpp)
target_link_libraries(test_transform PRIVATE
    Catch2::Catch2WithMain
    SJH::object               # PUBLIC include = src/ -> "object/transform.h" 접근
)
target_compile_features(test_transform PRIVATE cxx_std_17)
catch_discover_tests(test_transform)
```

그리고 파일 맨 아래 `add_custom_target(tests DEPENDS ...)` 의 DEPENDS 목록에 `test_transform`
을 추가한다.

- [ ] **Step 3: 재구성 후 빌드 — 실패 확인**

Run:
```bash
cmake --preset debug -DVCPKG_MANIFEST_INSTALL=OFF
cmake --build build_Darwin --target test_transform
```
Expected: **빌드 실패** — `error: no member named 'GetLocalMatrix' in 'SJH::Transform'`
(현재 `transform.h` 는 `GetModelMatrix` 만 가짐).

- [ ] **Step 4: `src/object/transform.h` 를 순수 값 객체로 교체**

`src/object/transform.h` 전체를 다음으로 교체한다:

```cpp
/**
 * @file transform.h
 * @brief 로컬 TRS 값 객체 + UV 변환. 계층/월드 합성은 SceneNode 가 담당.
 *
 * @details
 *  ### 책임
 *  - @ref SJH::Transform — TRS(Translate/Rotate/Scale) 값 + 로컬 모델 행렬 산출.
 *  - @ref SJH::UVTransform — UV 오프셋/스케일/회전을 묶은 경량 구조체.
 *
 *  ### 비-책임
 *  - ❌ 계층 구조 / 월드 합성 — @ref SJH::SceneNode 가 담당.
 *  - ❌ 렌더링 / GL uniform 전송 — Context::Render 가 담당.
 */

#ifndef __SJH_TRANSFORM_H__
#define __SJH_TRANSFORM_H__

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace SJH
{
    /**
     * @brief 로컬 TRS 값 객체.
     * @details 회전은 오일러 각(degree, X→Y→Z 순서) — 짐벌락 주의. 계층/월드 합성은
     *          @ref SceneNode 가 담당하며 본 클래스는 로컬 변환만 안다. 모든 멤버 public
     *          (Camera/Light 와 같은 POD-like 컨벤션).
     */
    class Transform
    {
    public:
        glm::vec3 Translate = glm::vec3(0.0f, 0.0f, 0.0f); ///< 이동량 (로컬 공간 offset).
        glm::vec3 EulerRot  = glm::vec3(0.0f, 0.0f, 0.0f); ///< 오일러 회전각 (degree, XYZ 순서).
        glm::vec3 Scale     = glm::vec3(1.0f, 1.0f, 1.0f); ///< 스케일 팩터.

        /**
         * @brief 로컬 모델 행렬 산출 — T·Rz·Ry·Rx·S 순서.
         * @return 부모를 고려하지 않은 로컬 변환 행렬.
         */
        glm::mat4 GetLocalMatrix() const
        {
            return glm::translate(glm::mat4(1.0f), Translate) *
                   glm::rotate(glm::mat4(1.0f), glm::radians(EulerRot[2]), glm::vec3(0.0f, 0.0f, 1.0f)) *
                   glm::rotate(glm::mat4(1.0f), glm::radians(EulerRot[1]), glm::vec3(0.0f, 1.0f, 0.0f)) *
                   glm::rotate(glm::mat4(1.0f), glm::radians(EulerRot[0]), glm::vec3(1.0f, 0.0f, 0.0f)) *
                   glm::scale(glm::mat4(1.0f), Scale);
        }
    };

    /// @brief UV 좌표계 변환 (오프셋 + 스케일 + 회전) 경량 구조체.
    class UVTransform
    {
    public:
        glm::vec2 Offset{0.0f, 0.0f}; ///< UV 오프셋 — 텍스처 스크롤 효과.
        glm::vec2 Scale{1.0f, 1.0f};  ///< UV 스케일 — 타일링 배수.
        float RotationDeg = 0.0f;     ///< UV 회전각 (degree).
    };
}
#endif //__SJH_TRANSFORM_H__
```

변경 요지: `INameTagInterface`·`Transform::Name`·`GetName()`·`Parent`·`Children`·
`GetModelMatrix()` 제거. `<glad/glad.h>`·`<glm/gtc/type_ptr.hpp>` include 제거.
`UVTransform` 무변경. `GetModelMatrix` 의 비-재귀 부분과 동일한 행렬을 `GetLocalMatrix` 가 산출.

- [ ] **Step 5: 빌드 + 테스트 — 통과 확인**

Run:
```bash
cmake --build build_Darwin --target test_transform
./build_Darwin/test/test_transform
cmake --build build_Darwin
```
Expected: `test_transform` 5개 케이스 전부 PASS. `cmake --build build_Darwin`(app+src) 도
성공 — `transform.h` 는 다른 곳에서 미사용이므로 회귀 없음.

- [ ] **Step 6: 커밋**

```bash
git add src/object/transform.h test/test_transform.cpp test/CMakeLists.txt
git commit -m "$(printf '%s\n' \
  '[refactor] : Transform 을 순수 로컬 TRS 값 객체로 축소' '' \
  'INameTagInterface/Parent/Children/GetModelMatrix 제거, GetLocalMatrix 신설.' \
  '계층 책임은 후속 SceneNode 로 이관. test_transform 5 케이스 추가.' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Task 2: `SceneNode` — 단일 노드 (월드 캐싱)

`SceneNode` 클래스를 신설하되 본 Task 는 *부모 없는 단일 노드* 동작만 — 로컬 변환 보관,
월드 행렬 캐싱, `Set*` 변경자. 계층(Attach/Detach)은 Task 3.

**Files:**
- Create: `src/object/scene_node.h`
- Create: `src/object/scene_node.cpp`
- Modify: `src/object/CMakeLists.txt`
- Create: `test/test_scene_node.cpp`
- Modify: `test/CMakeLists.txt`

- [ ] **Step 1: `src/object/scene_node.h` 작성**

본 Task 와 Task 3 이 함께 쓰는 *완성형* 헤더를 한 번에 작성한다 (Task 3 가 .cpp 만 추가):

```cpp
/**
 * @file scene_node.h
 * @brief intrusive 씬 그래프 노드 — Transform payload + 부모/자식 + 월드 행렬 캐싱.
 */
#ifndef __SJH_SCENE_NODE_H__
#define __SJH_SCENE_NODE_H__

#include "object/transform.h"
#include <glm/glm.hpp>
#include <vector>

namespace SJH
{
    /**
     * @brief intrusive 씬 그래프 노드.
     * @details 로컬 @ref Transform 을 payload 로 들고 부모/자식 포인터로 계층을 구성한다.
     *          월드 행렬은 캐싱하며 dirty 플래그로 무효화한다 (변경 시 자손까지 하향 전파).
     *          복사/이동 불가 — 다른 노드가 포인터로 참조하므로 주소가 고정되어야 한다.
     */
    class SceneNode
    {
    public:
        SceneNode() = default;
        ~SceneNode() = default;
        SceneNode(const SceneNode &) = delete;
        SceneNode &operator=(const SceneNode &) = delete;
        SceneNode(SceneNode &&) = delete;
        SceneNode &operator=(SceneNode &&) = delete;

        /// @brief 로컬 변환 읽기.
        const Transform &Local() const { return mLocal; }
        /// @brief 부모 노드 (루트면 nullptr).
        SceneNode *Parent() const { return mParent; }
        /// @brief 직속 자식 목록 (비소유 관찰자).
        const std::vector<SceneNode *> &Children() const { return mChildren; }

        /// @brief 월드 행렬 — dirty 면 재계산 후 캐시. parent 가 있으면 parent->World() 합성.
        glm::mat4 World() const;

        /// @brief 로컬 변환 일괄 교체 — 자신+자손 dirty.
        void SetLocal(const Transform &local);
        /// @brief 로컬 Translate 만 교체 — 자신+자손 dirty.
        void SetTranslate(const glm::vec3 &t);
        /// @brief 로컬 EulerRot 만 교체 — 자신+자손 dirty.
        void SetEulerRot(const glm::vec3 &r);
        /// @brief 로컬 Scale 만 교체 — 자신+자손 dirty.
        void SetScale(const glm::vec3 &s);

        /**
         * @brief @p child 를 이 노드의 자식으로 부착.
         * @details child 가 이미 부모를 가지면 먼저 분리한다. child 가 nullptr·자기 자신·
         *          이 노드의 조상이면 (순환) 아무 일도 하지 않는다.
         */
        void Attach(SceneNode *child);
        /// @brief @p child 를 자식 목록에서 분리 (child 의 부모를 nullptr 로).
        void Detach(SceneNode *child);

    private:
        void MarkSubtreeDirty();
        bool IsAncestorOf(const SceneNode *node) const;

        Transform mLocal;
        SceneNode *mParent = nullptr;             ///< 비소유 — SceneGraph 가 노드를 소유.
        std::vector<SceneNode *> mChildren;       ///< 비소유 관찰자.
        mutable glm::mat4 mWorldCache{1.0f};
        mutable bool mDirty = true;
    };
}
#endif // __SJH_SCENE_NODE_H__
```

- [ ] **Step 2: `src/object/scene_node.cpp` — 단일 노드 부분만 작성**

```cpp
#include "object/scene_node.h"
#include <algorithm>

namespace SJH
{
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

    void SceneNode::SetLocal(const Transform &local)
    {
        mLocal = local;
        MarkSubtreeDirty();
    }

    void SceneNode::SetTranslate(const glm::vec3 &t)
    {
        mLocal.Translate = t;
        MarkSubtreeDirty();
    }

    void SceneNode::SetEulerRot(const glm::vec3 &r)
    {
        mLocal.EulerRot = r;
        MarkSubtreeDirty();
    }

    void SceneNode::SetScale(const glm::vec3 &s)
    {
        mLocal.Scale = s;
        MarkSubtreeDirty();
    }

    void SceneNode::MarkSubtreeDirty()
    {
        mDirty = true;
        for (SceneNode *child : mChildren)
            child->MarkSubtreeDirty();
    }
}
```

> `Attach`/`Detach`/`IsAncestorOf` 정의는 Task 3 에서 추가한다. 본 Task 의 테스트는 이들을
> 호출하지 않으므로 링크 에러가 없다 (선언만 있고 미사용 = OK).

- [ ] **Step 3: `src/object/CMakeLists.txt` 에 소스 추가**

`src/object/CMakeLists.txt` 의 `add_library` 줄을 다음으로 교체:

```cmake
add_library(sjhopengl_object STATIC mesh.cpp model.cpp scene_node.cpp)
```

(링크 의존성 무변경 — `SceneNode` 는 glm 만 쓰며 `object` 모듈은 이미 `glm::glm` PUBLIC.)

- [ ] **Step 4: `test/test_scene_node.cpp` — 단일 노드 케이스 작성**

```cpp
/**
 * @file test_scene_node.cpp
 * @brief SJH::SceneNode — 월드 행렬 캐싱 + 계층 (GL 컨텍스트 불요).
 */
#include "object/scene_node.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/gtc/matrix_transform.hpp>

using Catch::Matchers::WithinAbs;

namespace
{
    void RequireMatNear(const glm::mat4 &a, const glm::mat4 &b, float eps = 1e-5f)
    {
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                REQUIRE_THAT(a[c][r], WithinAbs(b[c][r], eps));
    }
}

TEST_CASE("SceneNode 기본 — 부모 없는 노드의 World == 로컬 행렬", "[scene_node]")
{
    SJH::SceneNode node;
    RequireMatNear(node.World(), glm::mat4(1.0f));

    SJH::Transform t;
    t.Translate = glm::vec3(1.0f, 2.0f, 3.0f);
    node.SetLocal(t);
    RequireMatNear(node.World(), t.GetLocalMatrix());
}

TEST_CASE("SceneNode SetTranslate/SetScale — World 즉시 반영", "[scene_node]")
{
    SJH::SceneNode node;
    node.SetTranslate(glm::vec3(5.0f, 0.0f, 0.0f));
    node.SetScale(glm::vec3(2.0f, 2.0f, 2.0f));

    SJH::Transform expected;
    expected.Translate = glm::vec3(5.0f, 0.0f, 0.0f);
    expected.Scale = glm::vec3(2.0f, 2.0f, 2.0f);
    RequireMatNear(node.World(), expected.GetLocalMatrix());
}

TEST_CASE("SceneNode 캐시 — 변경 없이 두 번 호출하면 동일", "[scene_node]")
{
    SJH::SceneNode node;
    node.SetTranslate(glm::vec3(1.0f, 1.0f, 1.0f));
    const glm::mat4 first = node.World();
    const glm::mat4 second = node.World();
    RequireMatNear(first, second);
}
```

- [ ] **Step 5: `test/CMakeLists.txt` 에 테스트 등록**

다음 블록을 추가:

```cmake
#  test_scene_node — SceneNode 노드/계층 (GL 불요)
add_executable(test_scene_node test_scene_node.cpp)
target_link_libraries(test_scene_node PRIVATE
    Catch2::Catch2WithMain
    SJH::object
)
target_compile_features(test_scene_node PRIVATE cxx_std_17)
catch_discover_tests(test_scene_node)
```

`add_custom_target(tests DEPENDS ...)` 의 DEPENDS 목록에 `test_scene_node` 추가.

- [ ] **Step 6: 재구성 + 빌드 + 테스트**

Run:
```bash
cmake --preset debug -DVCPKG_MANIFEST_INSTALL=OFF
cmake --build build_Darwin --target test_scene_node
./build_Darwin/test/test_scene_node
cmake --build build_Darwin
```
Expected: `test_scene_node` 3개 케이스 PASS, `cmake --build build_Darwin` 성공.

- [ ] **Step 7: 커밋**

```bash
git add src/object/scene_node.h src/object/scene_node.cpp src/object/CMakeLists.txt \
        test/test_scene_node.cpp test/CMakeLists.txt
git commit -m "$(printf '%s\n' \
  '[feat] : SceneNode 신설 — 단일 노드 월드 행렬 캐싱' '' \
  'Transform payload + dirty-flag 캐싱. 계층(Attach/Detach)은 후속.' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Task 3: `SceneNode` — 계층 (Attach/Detach + dirty 하향 전파 + 순환 가드)

`SceneNode` 에 부모-자식 부착·분리, 월드 합성, dirty 하향 전파, 순환 가드를 추가한다.
헤더는 Task 2 에서 이미 완성형이므로 본 Task 는 `.cpp` 정의와 테스트만 추가한다.

**Files:**
- Modify: `src/object/scene_node.cpp`
- Modify: `test/test_scene_node.cpp`

- [ ] **Step 1: 실패하는 계층 테스트 추가**

`test/test_scene_node.cpp` 끝에 다음 케이스들을 추가한다:

```cpp
TEST_CASE("SceneNode 계층 — World 는 부모 합성", "[scene_node]")
{
    SJH::SceneNode root, child;
    root.SetTranslate(glm::vec3(10.0f, 0.0f, 0.0f));
    child.SetTranslate(glm::vec3(0.0f, 5.0f, 0.0f));
    root.Attach(&child);

    REQUIRE(child.Parent() == &root);
    RequireMatNear(child.World(), root.World() * child.Local().GetLocalMatrix());
}

TEST_CASE("SceneNode 하향 dirty 전파 — 부모 변경이 자식 World 갱신", "[scene_node]")
{
    SJH::SceneNode root, child;
    child.SetTranslate(glm::vec3(0.0f, 5.0f, 0.0f));
    root.Attach(&child);
    (void)child.World(); // 캐시 채움

    root.SetTranslate(glm::vec3(100.0f, 0.0f, 0.0f)); // 부모 이동
    RequireMatNear(child.World(), root.World() * child.Local().GetLocalMatrix());
}

TEST_CASE("SceneNode Detach — 분리 후 자식은 부모 영향 제거", "[scene_node]")
{
    SJH::SceneNode root, child;
    root.SetTranslate(glm::vec3(10.0f, 0.0f, 0.0f));
    child.SetTranslate(glm::vec3(0.0f, 5.0f, 0.0f));
    root.Attach(&child);
    root.Detach(&child);

    REQUIRE(child.Parent() == nullptr);
    RequireMatNear(child.World(), child.Local().GetLocalMatrix());
}

TEST_CASE("SceneNode 재부착 — 새 부모로 옮기면 옛 부모 자식목록서 제거", "[scene_node]")
{
    SJH::SceneNode a, b, child;
    a.Attach(&child);
    b.Attach(&child); // child 를 b 로 이동
    REQUIRE(child.Parent() == &b);
    REQUIRE(a.Children().empty());
    REQUIRE(b.Children().size() == 1);
}

TEST_CASE("SceneNode 순환 가드 — 조상을 자식으로 부착 시 거부", "[scene_node]")
{
    SJH::SceneNode root, mid, leaf;
    root.Attach(&mid);
    mid.Attach(&leaf);
    leaf.Attach(&root); // leaf 아래에 조상 root 부착 시도 — 거부되어야 함
    REQUIRE(root.Parent() == nullptr);   // root 는 여전히 부모 없음
    REQUIRE(leaf.Children().empty());    // leaf 는 자식 없음
}
```

- [ ] **Step 2: 빌드 — 실패 확인**

Run:
```bash
cmake --build build_Darwin --target test_scene_node
```
Expected: **링크 실패** — `undefined symbol: SJH::SceneNode::Attach` 등
(헤더에 선언만 있고 정의 없음).

- [ ] **Step 3: `src/object/scene_node.cpp` 에 계층 정의 추가**

`scene_node.cpp` 의 `MarkSubtreeDirty` 정의 *위에* (또는 `namespace SJH` 안 적절한 위치)
다음을 추가한다:

```cpp
    void SceneNode::Attach(SceneNode *child)
    {
        if (child == nullptr || child == this)
            return;
        // 순환 가드 — child 가 이 노드의 조상이면 부착 시 사이클이 생긴다.
        if (child->IsAncestorOf(this))
            return;
        // 이미 다른 부모가 있으면 먼저 분리.
        if (child->mParent != nullptr)
            child->mParent->Detach(child);
        child->mParent = this;
        mChildren.push_back(child);
        child->MarkSubtreeDirty();
    }

    void SceneNode::Detach(SceneNode *child)
    {
        if (child == nullptr)
            return;
        auto it = std::find(mChildren.begin(), mChildren.end(), child);
        if (it == mChildren.end())
            return;
        mChildren.erase(it);
        child->mParent = nullptr;
        child->MarkSubtreeDirty();
    }

    bool SceneNode::IsAncestorOf(const SceneNode *node) const
    {
        for (const SceneNode *cur = node; cur != nullptr; cur = cur->mParent)
            if (cur == this)
                return true;
        return false;
    }
```

- [ ] **Step 4: 빌드 + 테스트 — 통과 확인**

Run:
```bash
cmake --build build_Darwin --target test_scene_node
./build_Darwin/test/test_scene_node
```
Expected: `test_scene_node` 8개 케이스 전부 PASS.

- [ ] **Step 5: 커밋**

```bash
git add src/object/scene_node.cpp test/test_scene_node.cpp
git commit -m "$(printf '%s\n' \
  '[feat] : SceneNode 계층 — Attach/Detach + dirty 하향 전파 + 순환 가드' '' \
  'mParent/mChildren 동시 일관 유지. 부모 변경이 자손 World 캐시 무효화.' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Task 4: `SceneGraph<TId>` — enum 인덱싱 파사드

`std::array<SceneNode, Count>` 로 노드를 소유하고 enum 정수값으로 조회하는 템플릿 클래스.
템플릿이므로 헤더 온리 — 새 컴파일 단위·CMake 변경 없음.

**Files:**
- Create: `src/object/scene_graph.h`
- Create: `test/test_scene_graph.cpp`
- Modify: `test/CMakeLists.txt`

- [ ] **Step 1: 실패하는 테스트 작성**

`test/test_scene_graph.cpp` 생성:

```cpp
/**
 * @file test_scene_graph.cpp
 * @brief SJH::SceneGraph<TId> — enum 인덱싱 파사드 (GL 컨텍스트 불요).
 */
#include "object/scene_graph.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstddef>

using Catch::Matchers::WithinAbs;

namespace
{
    // SceneGraph 는 TId 가 Count 멤버를 가진 enum 이라는 점만 가정 — 테스트용 enum.
    enum class TestNode : std::size_t
    {
        Root = 0, A, B, C, Count
    };

    void RequireMatNear(const glm::mat4 &a, const glm::mat4 &b, float eps = 1e-5f)
    {
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                REQUIRE_THAT(a[c][r], WithinAbs(b[c][r], eps));
    }
}

TEST_CASE("SceneGraph 기본 — 모든 노드 World == identity", "[scene_graph]")
{
    SJH::SceneGraph<TestNode> graph;
    RequireMatNear(graph.World(TestNode::Root), glm::mat4(1.0f));
    RequireMatNear(graph.World(TestNode::C), glm::mat4(1.0f));
}

TEST_CASE("SceneGraph SetLocal — enum 으로 노드 변환 설정", "[scene_graph]")
{
    SJH::SceneGraph<TestNode> graph;
    SJH::Transform t;
    t.Translate = glm::vec3(3.0f, 0.0f, 0.0f);
    graph.SetLocal(TestNode::A, t);
    RequireMatNear(graph.World(TestNode::A), t.GetLocalMatrix());
}

TEST_CASE("SceneGraph Attach — 계층 World 합성", "[scene_graph]")
{
    SJH::SceneGraph<TestNode> graph;
    graph.At(TestNode::Root).SetTranslate(glm::vec3(10.0f, 0.0f, 0.0f));
    graph.At(TestNode::A).SetTranslate(glm::vec3(0.0f, 2.0f, 0.0f));
    graph.At(TestNode::B).SetTranslate(glm::vec3(0.0f, 0.0f, 1.0f));
    graph.Attach(TestNode::A, TestNode::Root);
    graph.Attach(TestNode::B, TestNode::A);

    RequireMatNear(graph.World(TestNode::B),
                   graph.World(TestNode::A) * graph.At(TestNode::B).Local().GetLocalMatrix());
}

TEST_CASE("SceneGraph 하향 dirty 전파 — 조상 변경이 자손 World 갱신", "[scene_graph]")
{
    SJH::SceneGraph<TestNode> graph;
    graph.At(TestNode::A).SetTranslate(glm::vec3(0.0f, 2.0f, 0.0f));
    graph.Attach(TestNode::A, TestNode::Root);
    (void)graph.World(TestNode::A);

    graph.At(TestNode::Root).SetTranslate(glm::vec3(50.0f, 0.0f, 0.0f));
    RequireMatNear(graph.World(TestNode::A),
                   graph.World(TestNode::Root) * graph.At(TestNode::A).Local().GetLocalMatrix());
}

TEST_CASE("SceneGraph Detach — enum 으로 분리", "[scene_graph]")
{
    SJH::SceneGraph<TestNode> graph;
    graph.At(TestNode::Root).SetTranslate(glm::vec3(10.0f, 0.0f, 0.0f));
    graph.Attach(TestNode::A, TestNode::Root);
    graph.Detach(TestNode::A);
    REQUIRE(graph.At(TestNode::A).Parent() == nullptr);
}
```

- [ ] **Step 2: `test/CMakeLists.txt` 에 등록**

```cmake
#  test_scene_graph — SceneGraph enum 인덱싱 파사드 (GL 불요)
add_executable(test_scene_graph test_scene_graph.cpp)
target_link_libraries(test_scene_graph PRIVATE
    Catch2::Catch2WithMain
    SJH::object
)
target_compile_features(test_scene_graph PRIVATE cxx_std_17)
catch_discover_tests(test_scene_graph)
```

`add_custom_target(tests DEPENDS ...)` 의 DEPENDS 에 `test_scene_graph` 추가.

- [ ] **Step 3: 재구성 후 빌드 — 실패 확인**

Run:
```bash
cmake --preset debug -DVCPKG_MANIFEST_INSTALL=OFF
cmake --build build_Darwin --target test_scene_graph
```
Expected: **빌드 실패** — `fatal error: 'object/scene_graph.h' file not found`.

- [ ] **Step 4: `src/object/scene_graph.h` 작성**

```cpp
/**
 * @file scene_graph.h
 * @brief enum 인덱스 기반 씬 그래프 — 노드 소유 + enum→노드 O(1) 조회 파사드.
 */
#ifndef __SJH_SCENE_GRAPH_H__
#define __SJH_SCENE_GRAPH_H__

#include "object/scene_node.h"
#include <array>
#include <cstddef>

namespace SJH
{
    /**
     * @brief enum 인덱스 기반 씬 그래프.
     * @tparam TId 마지막 항목이 `Count` 인 enum — 정수값이 노드 배열 인덱스.
     * @details 모든 @ref SceneNode 를 std::array 로 소유한다. 배열이라 노드 주소가
     *          고정되므로 노드 간 부모/자식 포인터가 안전하다. SceneNode 가 복사/이동
     *          불가이므로 SceneGraph 도 복사/이동 불가 — 소유자가 멤버로 in-place 생성한다.
     */
    template <typename TId>
    class SceneGraph
    {
    public:
        /// @brief 노드 개수 — TId::Count 의 정수값.
        static constexpr std::size_t Count = static_cast<std::size_t>(TId::Count);

        /// @brief @p id 노드 참조 (변경 가능).
        SceneNode &At(TId id) { return mNodes[Index(id)]; }
        /// @brief @p id 노드 참조 (읽기 전용).
        const SceneNode &At(TId id) const { return mNodes[Index(id)]; }

        /// @brief @p id 노드의 월드 행렬.
        glm::mat4 World(TId id) const { return mNodes[Index(id)].World(); }

        /// @brief @p id 노드의 로컬 변환 일괄 교체.
        void SetLocal(TId id, const Transform &local) { mNodes[Index(id)].SetLocal(local); }

        /// @brief @p child 를 @p parent 의 자식으로 부착.
        void Attach(TId child, TId parent) { mNodes[Index(parent)].Attach(&mNodes[Index(child)]); }

        /// @brief @p child 를 현재 부모에서 분리.
        void Detach(TId child)
        {
            SceneNode &node = mNodes[Index(child)];
            if (node.Parent() != nullptr)
                node.Parent()->Detach(&node);
        }

    private:
        static std::size_t Index(TId id) { return static_cast<std::size_t>(id); }

        std::array<SceneNode, Count> mNodes;
    };
}
#endif // __SJH_SCENE_GRAPH_H__
```

- [ ] **Step 5: 빌드 + 테스트 — 통과 확인**

Run:
```bash
cmake --build build_Darwin --target test_scene_graph
./build_Darwin/test/test_scene_graph
```
Expected: `test_scene_graph` 5개 케이스 전부 PASS.

- [ ] **Step 6: 커밋**

```bash
git add src/object/scene_graph.h test/test_scene_graph.cpp test/CMakeLists.txt
git commit -m "$(printf '%s\n' \
  '[feat] : SceneGraph<TId> 신설 — enum 정수 인덱싱 씬 그래프' '' \
  'std::array<SceneNode,Count> 소유, enum 값으로 O(1) 조회. 헤더 온리 템플릿.' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Task 5: `SceneNodeId` enum + Context Init 씬 그래프 구성

`Context` 씬 노드 enum 을 정의하고 `Context` 에 `SceneGraph` 멤버를 추가, `Init()` 에서
씬 그래프를 1회 구성한다. 본 Task 후에도 `Render()` 는 아직 inline `modelTransform` 을 쓰므로
*동작은 불변* (그래프는 구성되지만 미사용) — 안전한 중간 커밋.

**Files:**
- Create: `src/context/scene_node_id.h`
- Modify: `src/context/context.h`
- Modify: `src/context/context.cpp` (`Init()`)

- [ ] **Step 1: `src/context/scene_node_id.h` 작성**

```cpp
/**
 * @file scene_node_id.h
 * @brief Context 씬의 노드 식별 enum — SceneGraph 의 배열 인덱스.
 */
#ifndef __SJH_SCENE_NODE_ID_H__
#define __SJH_SCENE_NODE_ID_H__

#include <cstddef>

namespace SJH
{
    /// @brief Context 씬 노드 식별자. 정수값이 SceneGraph 노드 배열 인덱스.
    enum class SceneNodeId : std::size_t
    {
        Root = 0,
        Plane,
        Box1,
        Box2,
        Outline,
        Windows,
        Window0,
        Window1,
        Window2,
        PointMarker0,
        PointMarker1,
        SpotMarker,
        Count
    };
}
#endif // __SJH_SCENE_NODE_ID_H__
```

- [ ] **Step 2: `src/context/context.h` 에 include + 멤버 추가**

`context.h` 의 include 블록 (현재 `#include "shader/shader.h"` 다음 줄) 에 추가:

```cpp
#include "object/scene_graph.h"
#include "context/scene_node_id.h"
```

그리고 멤버 선언부 — `Camera mCamera;` (현재 147행) 선언 *다음* 에 추가:

```cpp
        /// @brief 씬 그래프 — 드로우 대상의 로컬 변환 + 계층. Render 가 World(id) 로 월드 행렬 산출.
        SceneGraph<SceneNodeId> mScene;
```

- [ ] **Step 3: `Context::Init()` 에 씬 그래프 구성 추가**

`context.cpp` `Init()` 의 `return true;` (현재 646행) *바로 앞* 에 다음 블록을 추가:

```cpp
        // === Scene Graph 구성 ===
        // 정적 노드의 로컬 변환 + 계층. 동적 노드(마커)는 Render() 가 매 프레임 갱신.
        mScene.SetLocal(SceneNodeId::Plane,
                        Transform{.Translate = {0.0f, -0.5f, 0.0f},
                                  .Scale = {10.0f, 1.0f, 10.0f}});
        mScene.SetLocal(SceneNodeId::Box1,
                        Transform{.Translate = {-1.0f, 0.75f, -4.0f},
                                  .EulerRot = {0.0f, 30.0f, 0.0f},
                                  .Scale = {1.5f, 1.5f, 1.5f}});
        mScene.SetLocal(SceneNodeId::Box2,
                        Transform{.Translate = {0.0f, 0.75f, 2.0f},
                                  .EulerRot = {0.0f, 20.0f, 0.0f},
                                  .Scale = {1.5f, 1.5f, 1.5f}});
        // Outline 은 Box2 자식 — World(Outline) = World(Box2) * scale(1.05).
        mScene.SetLocal(SceneNodeId::Outline,
                        Transform{.Scale = {1.05f, 1.05f, 1.05f}});
        // Windows 그룹은 identity, 창들은 절대 좌표를 로컬로 — 그룹 이동 시 함께 이동.
        mScene.SetLocal(SceneNodeId::Window0, Transform{.Translate = {0.0f, 0.5f, 4.0f}});
        mScene.SetLocal(SceneNodeId::Window1, Transform{.Translate = {0.2f, 0.5f, 5.0f}});
        mScene.SetLocal(SceneNodeId::Window2, Transform{.Translate = {0.4f, 0.5f, 6.0f}});
        // 마커는 스케일만 고정 — 위치는 Render() 가 광원 위치로 매 프레임 갱신.
        mScene.SetLocal(SceneNodeId::PointMarker0, Transform{.Scale = {0.1f, 0.1f, 0.1f}});
        mScene.SetLocal(SceneNodeId::PointMarker1, Transform{.Scale = {0.1f, 0.1f, 0.1f}});
        mScene.SetLocal(SceneNodeId::SpotMarker,   Transform{.Scale = {0.1f, 0.1f, 0.1f}});

        mScene.Attach(SceneNodeId::Plane,        SceneNodeId::Root);
        mScene.Attach(SceneNodeId::Box1,         SceneNodeId::Root);
        mScene.Attach(SceneNodeId::Box2,         SceneNodeId::Root);
        mScene.Attach(SceneNodeId::Outline,      SceneNodeId::Box2);
        mScene.Attach(SceneNodeId::Windows,      SceneNodeId::Root);
        mScene.Attach(SceneNodeId::Window0,      SceneNodeId::Windows);
        mScene.Attach(SceneNodeId::Window1,      SceneNodeId::Windows);
        mScene.Attach(SceneNodeId::Window2,      SceneNodeId::Windows);
        mScene.Attach(SceneNodeId::PointMarker0, SceneNodeId::Root);
        mScene.Attach(SceneNodeId::PointMarker1, SceneNodeId::Root);
        mScene.Attach(SceneNodeId::SpotMarker,   SceneNodeId::Root);
```

> `Transform{.Translate = ...}` 지정 초기화는 `Init()` 의 기존 `mDirLight = {.Direction=...}`
> (501행~) 와 같은 양식 — 이 프로젝트 빌드에서 이미 통과하는 패턴이다.

- [ ] **Step 4: 빌드 — 통과 확인**

Run:
```bash
cmake --build build_Darwin
```
Expected: 빌드 성공. `mScene` 는 아직 `Render()` 에서 미사용 (경고 없음 — 멤버 변수).
실행 시 화면은 현행과 완전 동일 (그래프는 구성만 됨).

- [ ] **Step 5: 커밋**

```bash
git add src/context/scene_node_id.h src/context/context.h src/context/context.cpp
git commit -m "$(printf '%s\n' \
  '[feat] : Context 에 SceneGraph 도입 — Init 에서 씬 그래프 구성' '' \
  'SceneNodeId enum 정의, mScene 멤버 추가. Render 적용은 후속 — 동작 불변.' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Task 6: `Context::Render()` — inline `modelTransform` 을 `World(id)` 로 교체

`Render()` 의 inline 변환 생성을 `mScene.World(id)` 호출로 교체한다. 산출 행렬이 수학적으로
동일하므로 시각 출력은 불변이어야 한다.

**Files:**
- Modify: `src/context/context.cpp` (`Render()`)

- [ ] **Step 1: 마커 3개 블록 교체**

`context.cpp` 의 마커 루프 (현재 283~293행) 를 다음으로 교체:

```cpp
            const SceneNodeId markerIds[3] = {
                SceneNodeId::PointMarker0, SceneNodeId::PointMarker1, SceneNodeId::SpotMarker};
            for (int i = 0; i < 3; ++i)
            {
                if (mFlashLightMode && i >= 2)
                    continue;
                // 동적 노드 — 광원 위치를 매 프레임 로컬 Translate 로 갱신.
                mScene.At(markerIds[i]).SetTranslate(markerPositions[i]);
                Uniforms::SetVec4(*mSimpleProgram.get(), Const::UNI_BASE_COLOR,
                                  glm::vec4(markerColors[i], 1.0f));
                Uniforms::SetMat4(*mSimpleProgram.get(), Const::UNI_TRANSFORM_MAT,
                                  projMat * viewMat * mScene.World(markerIds[i]));
                mBox->Draw();
            }
```

- [ ] **Step 2: plane 블록 교체**

plane 드로우 블록 (현재 334~344행) 의 `modelTransform` 정의를 교체:

```cpp
            {
                auto modelTransform = mScene.World(SceneNodeId::Plane);
                auto transform = projMat * viewMat * modelTransform;
                Uniforms::SetMat4(*mProgram, Const::UNI_TRANSFORM_MAT, transform);
                Uniforms::SetMat4(*mProgram, Const::UNI_MODEL_TRANSFORM_MAT, modelTransform);
                auto planeMaterial = mRM->FindMaterial(Const::STR_MATERIAL_PLANE);
                planeMaterial->Apply();
                mBox->Draw();
            }
```

- [ ] **Step 3: box1 블록 교체**

box1 드로우 블록 (현재 346~357행) 의 `modelTransform` 정의를 교체:

```cpp
            {
                auto modelTransform = mScene.World(SceneNodeId::Box1);
                auto transform = projMat * viewMat * modelTransform;
                Uniforms::SetMat4(*mProgram, Const::UNI_TRANSFORM_MAT, transform);
                Uniforms::SetMat4(*mProgram, Const::UNI_MODEL_TRANSFORM_MAT, modelTransform);
                auto box1Material = mRM->FindMaterial(Const::STR_MATERIAL_BOX1);
                box1Material->Apply();
                mBox->Draw();
            }
```

- [ ] **Step 4: box2 블록 교체**

box2 본체 드로우 블록 (현재 366~378행) 의 `modelTransform` 정의를 교체:

```cpp
            {
                auto modelTransform = mScene.World(SceneNodeId::Box2);
                auto transform = projMat * viewMat * modelTransform;
                Uniforms::SetMat4(*mProgram, Const::UNI_TRANSFORM_MAT, transform);
                Uniforms::SetMat4(*mProgram, Const::UNI_MODEL_TRANSFORM_MAT, modelTransform);

                auto box2Material = mRM->FindMaterial(Const::STR_MATERIAL_BOX2);
                box2Material->Apply();
                mBox->Draw();
            }
```

- [ ] **Step 5: outline 블록 교체**

outline 드로우 블록 (현재 388~398행) 을 교체 — Outline 노드가 이미 1.05 스케일을 들고
Box2 자식이므로 `* glm::scale(1.05)` 를 제거한다:

```cpp
            {
                auto transform = projMat * viewMat * mScene.World(SceneNodeId::Outline);
                Uniforms::SetVec4(*mSimpleProgram.get(), Const::UNI_BASE_COLOR,
                                  glm::vec4(1.0f, 1.0f, 0.5f, 1.0f));
                Uniforms::SetMat4(*mSimpleProgram.get(), Const::UNI_TRANSFORM_MAT, transform);
                mBox->Draw();
            }
```

- [ ] **Step 6: window 3개 블록 교체**

window 드로우 블록 (현재 422~438행) 의 세 `modelTransform`/`transform` 쌍을 교체:

```cpp
            auto modelTransform = mScene.World(SceneNodeId::Window0);
            auto transform = projMat * viewMat * modelTransform;
            Uniforms::SetMat4(*mTextureProgram, Const::UNI_TRANSFORM_MAT, transform);
            mPlane->Draw();

            modelTransform = mScene.World(SceneNodeId::Window1);
            transform = projMat * viewMat * modelTransform;
            Uniforms::SetMat4(*mTextureProgram, Const::UNI_TRANSFORM_MAT, transform);
            mPlane->Draw();

            modelTransform = mScene.World(SceneNodeId::Window2);
            transform = projMat * viewMat * modelTransform;
            Uniforms::SetMat4(*mTextureProgram, Const::UNI_TRANSFORM_MAT, transform);
            mPlane->Draw();
```

> 현재 328~331행의 스포트라이트 uniform 셋업 블록(`modelTransform = glm::mat4(1.0f)`)은
> *드로우 대상이 아니므로* 교체 대상이 아니다 — 그대로 둔다.

- [ ] **Step 7: 빌드 + 실행 — 시각 회귀 확인**

Run:
```bash
cmake --build build_Darwin
./build_Darwin/OpenGL-With-CMake
```
Expected: 빌드 성공. 실행 화면이 교체 전과 동일 — 평면/박스1/박스2/아웃라인/창3개/광원
마커가 같은 위치·크기·회전으로 렌더된다. (산출 행렬이 수학적으로 동일하므로 불변.)
확인 후 창을 닫는다.

- [ ] **Step 8: 회귀 테스트 + 커밋**

Run:
```bash
cmake --build build_Darwin --target tests
ctest --test-dir build_Darwin --output-on-failure
```
Expected: 전체 테스트 통과.

```bash
git add src/context/context.cpp
git commit -m "$(printf '%s\n' \
  '[refactor] : Context::Render 의 inline modelTransform 을 SceneGraph 로 교체' '' \
  '드로우 8곳을 mScene.World(id) 호출로 대체. Outline 은 Box2 자식으로 자동 추종.' \
  '산출 행렬 동일 — 시각 출력 불변.' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Task 7: 문서 갱신

모듈 인벤토리와 doxygen 클래스 그래프를 신규 클래스에 맞춰 갱신한다.

**Files:**
- Modify: `.claude/MEMORY.md`
- Modify: `.claude/architecture.md`
- Modify: `doc/pages/00-mainpage.md` (`doxygen-class-graph` skill 경유)

- [ ] **Step 1: `.claude/MEMORY.md` 모듈 인벤토리 갱신**

`src/object/` 항목 설명에 `SceneNode` / `SceneGraph` / `Transform`(값 객체화) 를 반영한다.
`object` 모듈 줄을 다음 취지로 수정: "mesh.cpp/model.cpp/scene_node.cpp + camera.h/light.h/
transform.h/scene_node.h/scene_graph.h. Transform=로컬 TRS 값 객체, SceneNode=intrusive
노드 트리, SceneGraph<TId>=enum 인덱싱 씬 그래프."

- [ ] **Step 2: `.claude/architecture.md` 갱신**

`src/object/` 모듈 인벤토리 절에 `Transform`(값 객체로 축소 — 계층 책임 SceneNode 이관),
`SceneNode`(intrusive 트리 + 월드 캐싱), `SceneGraph<TId>`(enum 인덱싱) 항목을 추가/수정한다.
설계 배경은 [doc/design/2026-05-19-transform-scene-graph-design.md](2026-05-19-transform-scene-graph-design.md)
를 참조하도록 링크.

- [ ] **Step 3: doxygen 클래스 그래프 갱신**

`doxygen-class-graph` skill 을 호출해 `doc/pages/00-mainpage.md` 의 클래스 의존 그래프를
`Transform` 축소 · `SceneNode` · `SceneGraph<TId>` 신설에 맞춰 갱신한다.

- [ ] **Step 4: 커밋**

```bash
git add .claude/MEMORY.md .claude/architecture.md doc/pages/00-mainpage.md
git commit -m "$(printf '%s\n' \
  '[doc] : Transform/SceneNode/SceneGraph — 모듈 인벤토리 + doxygen 그래프 갱신' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Self-Review 결과

- **Spec 커버리지:** 설계 §3(Transform 축소)=Task1, §4(SceneNode)=Task2+3, §5(SceneGraph)=
  Task4, §6(SceneNodeId+계층)=Task5, §7(Context 적용)=Task5+6, §8(테스트)=Task1/2/3/4 +
  Task6 시각 회귀, §9(영향 파일/문서)=Task7. 설계 §12 Phase 분해와 1:1 대응. 누락 없음.
- **Placeholder:** 없음 — 모든 코드 블록은 실제 코드. CMake 수정은 정확한 교체 줄 명시.
- **타입 일관성:** `Transform::GetLocalMatrix()`, `SceneNode::{World,SetLocal,SetTranslate,
  SetEulerRot,SetScale,Attach,Detach,Parent,Children,Local}`, `SceneGraph<TId>::{At,World,
  SetLocal,Attach,Detach,Count}`, `SceneNodeId` — Task 간 시그니처·이름 일치 확인 완료.
- **빌드 green 불변식:** 각 Task 종료 시 `cmake --build build_Darwin`(app+src) 성공 —
  Task1(Transform 미사용), Task2~4(신규 파일, 앱 미사용), Task5(mScene 멤버만), Task6
  (Render 교체, 시각 불변). 테스트는 EXCLUDE_FROM_ALL 이라 명시 빌드.
