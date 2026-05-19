# Camera + Light → Scene Graph 통합 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `Camera` 와 모든 라이트의 위치·방향을 Scene Graph 노드의 `Transform` 으로 편입한다 — 노드가 단일 소유자, `Camera`/`Light` 클래스는 비-변환 데이터만 보유.

**Architecture:** `SceneNode` 에 `WorldForward()`/`TranslateBy()` 를 더한다. `SceneNodeId` 에 `Camera`/`DirLight` 노드를 추가하고 마커 노드를 라이트 노드로 리네임한다. `Camera` 는 투영 렌즈로 축소(view 행렬은 `inverse(World(Camera))`), `Light` 구조체는 색상/감쇠/cutoff 만 남기고 `Uniforms::Set*Light` 가 위치/방향을 명시 인자로 받는다. `Context` 의 Init·Render·ImGui·입력 람다가 노드 기반으로 재배선된다.

**Tech Stack:** C++17, CMake + Make, vcpkg, Catch2 v3, glm, OpenGL/glad

**설계 출처:** [doc/design/2026-05-19-camera-light-scene-graph-design.md](2026-05-19-camera-light-scene-graph-design.md)

**공통 빌드/테스트 명령:**
```bash
cmake --preset debug -DVCPKG_MANIFEST_INSTALL=OFF   # CMakeLists 변경 시
cmake --build build_Darwin                          # 코드만 수정
cmake --build build_Darwin --target test_scene_node # 특정 테스트 빌드
./build_Darwin/test/test_scene_node                 # 테스트 실행
```

> 본 계획은 CMakeLists 를 *변경하지 않는다* (새 파일·새 타깃 없음 — 기존 파일 수정만). 따라서
> 재구성(`cmake --preset`)은 불필요하고 `cmake --build build_Darwin` 만으로 충분하다.

---

## Task 1: `SceneNode` / `SceneGraph` 보강 — `WorldForward` / `TranslateBy`

노드의 월드 전방 벡터 접근자와 증분 이동을 추가한다. 소비자(Context) 변경 없이 독립적으로 검증된다.

**Files:**
- Modify: `src/object/scene_node.h`
- Modify: `src/object/scene_node.cpp`
- Modify: `src/object/scene_graph.h`
- Modify: `test/test_scene_node.cpp`
- Modify: `test/test_scene_graph.cpp`

- [ ] **Step 1: 실패하는 테스트 추가 — `test/test_scene_node.cpp`**

`test/test_scene_node.cpp` 끝에 다음 케이스들을 추가한다:

```cpp
TEST_CASE("SceneNode WorldForward — 기본은 -Z", "[scene_node]")
{
    SJH::SceneNode node;
    const glm::vec3 f = node.WorldForward();
    REQUIRE_THAT(f.x, WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(f.y, WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(f.z, WithinAbs(-1.0f, 1e-5f));
}

TEST_CASE("SceneNode WorldForward — yaw 90도면 -X", "[scene_node]")
{
    SJH::SceneNode node;
    node.SetEulerRot(glm::vec3(0.0f, 90.0f, 0.0f));
    const glm::vec3 f = node.WorldForward();
    REQUIRE_THAT(f.x, WithinAbs(-1.0f, 1e-5f));
    REQUIRE_THAT(f.y, WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(f.z, WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("SceneNode WorldForward — pitch -90도면 -Y", "[scene_node]")
{
    SJH::SceneNode node;
    node.SetEulerRot(glm::vec3(-90.0f, 0.0f, 0.0f));
    const glm::vec3 f = node.WorldForward();
    REQUIRE_THAT(f.x, WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(f.y, WithinAbs(-1.0f, 1e-5f));
    REQUIRE_THAT(f.z, WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("SceneNode WorldForward — 부모 회전 반영", "[scene_node]")
{
    SJH::SceneNode root, child;
    root.SetEulerRot(glm::vec3(0.0f, 90.0f, 0.0f));
    root.Attach(&child);
    const glm::vec3 f = child.WorldForward(); // child 로컬 identity, 부모 yaw 90
    REQUIRE_THAT(f.x, WithinAbs(-1.0f, 1e-5f));
    REQUIRE_THAT(f.z, WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("SceneNode TranslateBy — 증분 누적", "[scene_node]")
{
    SJH::SceneNode node;
    node.SetTranslate(glm::vec3(1.0f, 0.0f, 0.0f));
    node.TranslateBy(glm::vec3(2.0f, 3.0f, 0.0f));
    const glm::mat4 w = node.World();
    REQUIRE_THAT(w[3][0], WithinAbs(3.0f, 1e-5f));
    REQUIRE_THAT(w[3][1], WithinAbs(3.0f, 1e-5f));
}

TEST_CASE("SceneNode TranslateBy — 자식 World 갱신 (dirty 전파)", "[scene_node]")
{
    SJH::SceneNode root, child;
    root.Attach(&child);
    (void)child.World();
    root.TranslateBy(glm::vec3(5.0f, 0.0f, 0.0f));
    const glm::mat4 cw = child.World();
    REQUIRE_THAT(cw[3][0], WithinAbs(5.0f, 1e-5f));
}
```

- [ ] **Step 2: 실패하는 테스트 추가 — `test/test_scene_graph.cpp`**

`test/test_scene_graph.cpp` 끝에 추가:

```cpp
TEST_CASE("SceneGraph WorldForward — enum 으로 노드 전방", "[scene_graph]")
{
    SJH::SceneGraph<TestNode> graph;
    graph.At(TestNode::A).SetEulerRot(glm::vec3(0.0f, 90.0f, 0.0f));
    const glm::vec3 f = graph.WorldForward(TestNode::A);
    REQUIRE_THAT(f.x, WithinAbs(-1.0f, 1e-5f));
    REQUIRE_THAT(f.z, WithinAbs(0.0f, 1e-5f));
}
```

- [ ] **Step 3: 빌드 — 실패 확인**

Run:
```bash
cmake --build build_Darwin --target test_scene_node test_scene_graph
```
Expected: **빌드 실패** — `no member named 'WorldForward'` / `'TranslateBy'` in `SJH::SceneNode`.

- [ ] **Step 4: `src/object/scene_node.h` 에 선언 추가**

`World()` 선언 다음 줄에 추가:

```cpp
        /// @brief 노드의 월드 전방 단위 벡터 — 회전된 -Z. 카메라 front·라이트 방향에 사용.
        glm::vec3 WorldForward() const;
```

`SetScale(...)` 선언 다음 줄에 추가:

```cpp
        /// @brief 로컬 Translate 를 @p delta 만큼 증분 — 자신+자손 dirty.
        void TranslateBy(const glm::vec3 &delta);
```

- [ ] **Step 5: `src/object/scene_node.cpp` 에 정의 추가**

`World()` 정의 다음에 추가:

```cpp
    glm::vec3 SceneNode::WorldForward() const
    {
        return glm::normalize(glm::vec3(World() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
    }
```

`SetScale(...)` 정의 다음에 추가:

```cpp
    void SceneNode::TranslateBy(const glm::vec3 &delta)
    {
        mLocal.Translate += delta;
        MarkSubtreeDirty();
    }
```

- [ ] **Step 6: `src/object/scene_graph.h` 에 위임 메서드 추가**

`World(TId id)` 메서드 다음 줄에 추가:

```cpp
        /// @brief @p id 노드의 월드 전방 벡터.
        glm::vec3 WorldForward(TId id) const { return mNodes[Index(id)].WorldForward(); }
```

- [ ] **Step 7: 빌드 + 테스트 — 통과 확인**

Run:
```bash
cmake --build build_Darwin --target test_scene_node test_scene_graph
./build_Darwin/test/test_scene_node
./build_Darwin/test/test_scene_graph
cmake --build build_Darwin
```
Expected: 두 테스트 전부 PASS. `cmake --build build_Darwin`(app+src) 성공.

- [ ] **Step 8: 커밋**

```bash
git add src/object/scene_node.h src/object/scene_node.cpp src/object/scene_graph.h \
        test/test_scene_node.cpp test/test_scene_graph.cpp
git commit -m "$(printf '%s\n' \
  '[feat] : SceneNode 보강 — WorldForward / TranslateBy' '' \
  'WorldForward = 회전된 -Z 월드 전방 (카메라 front·라이트 방향용).' \
  'TranslateBy = 증분 이동 (카메라 WASD용). SceneGraph 위임 추가.' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Task 2: `SceneNodeId` 확장 + 마커 노드 리네임

`Camera`/`DirLight` 노드를 추가하고 마커 노드를 라이트 노드로 리네임한다. 순수 리네임 + enum
추가 — 동작 불변 (새 enum 값은 아직 미사용, `SceneGraph` 배열만 2칸 늘어남).

**Files:**
- Modify: `src/context/scene_node_id.h`
- Modify: `src/context/context.cpp`

- [ ] **Step 1: `src/context/scene_node_id.h` enum 교체**

`enum class SceneNodeId` 본문을 다음으로 교체:

```cpp
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
        Camera,
        DirLight,
        PointLight0,
        PointLight1,
        SpotLight,
        Count
    };
```

- [ ] **Step 2: `context.cpp` — 마커 ID 배열 리네임**

`context.cpp` 의 `markerIds` 선언 (현재 229~230행) 을 교체:

```cpp
            const SceneNodeId markerIds[3] = {
                SceneNodeId::PointLight0, SceneNodeId::PointLight1, SceneNodeId::SpotLight};
```

- [ ] **Step 3: `context.cpp` — Scene Graph 구성의 마커 노드 리네임**

`Init()` 의 마커 `SetLocal` 3줄 (현재 651~653행) 을 교체:

```cpp
        mScene.SetLocal(SceneNodeId::PointLight0, Transform{.Scale = {0.1f, 0.1f, 0.1f}});
        mScene.SetLocal(SceneNodeId::PointLight1, Transform{.Scale = {0.1f, 0.1f, 0.1f}});
        mScene.SetLocal(SceneNodeId::SpotLight, Transform{.Scale = {0.1f, 0.1f, 0.1f}});
```

마커 `Attach` 3줄 (현재 663~665행) 을 교체:

```cpp
        mScene.Attach(SceneNodeId::PointLight0, SceneNodeId::Root);
        mScene.Attach(SceneNodeId::PointLight1, SceneNodeId::Root);
        mScene.Attach(SceneNodeId::SpotLight, SceneNodeId::Root);
```

- [ ] **Step 4: 빌드 — 통과 확인**

Run:
```bash
cmake --build build_Darwin
```
Expected: 빌드 성공. `PointMarker`/`SpotMarker` 잔존 참조가 있으면 컴파일 에러 — 그 경우
해당 참조를 새 이름으로 교체 (위 3개 지점이 전부여야 함). 실행 시 화면 불변.

- [ ] **Step 5: 커밋**

```bash
git add src/context/scene_node_id.h src/context/context.cpp
git commit -m "$(printf '%s\n' \
  '[refactor] : SceneNodeId 에 Camera/DirLight 추가 + 마커 노드 리네임' '' \
  'PointMarker0/1→PointLight0/1, SpotMarker→SpotLight. Camera/DirLight enum 추가.' \
  '순수 리네임 — 동작 불변.' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Task 3: `Camera` 축소 + Context 카메라 재배선

`Camera` 를 투영 렌즈로 축소하고, 카메라 위치·방향·뷰를 `SceneNodeId::Camera` 노드 기반으로
교체한다.

**Files:**
- Modify: `src/object/camera.h`
- Modify: `src/context/context.cpp`

- [ ] **Step 1: `src/object/camera.h` 를 렌즈 전용으로 교체**

`camera.h` 전체를 다음으로 교체:

```cpp
#ifndef __SJH_CAMERA_H__
#define __SJH_CAMERA_H__

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace SJH
{
    /**
     * @brief 원근 투영 *렌즈* — Fov / 종횡비 / 클리핑 평면 + projection 행렬 산출.
     *
     * @details
     *  ### 책임
     *  - 원근 투영 파라미터 보관 + @ref GetProjMatrix 산출.
     *
     *  ### 비-책임
     *  - ❌ 카메라 *배치* (위치/방향) — `SceneNodeId::Camera` 노드의 @ref Transform 이 소유.
     *    view 행렬은 `inverse(SceneGraph::World(Camera))` 로 산출 (Context).
     *  - ❌ 입력 처리 — 입력 모듈이 카메라 노드를 갱신.
     *
     *  Unity `Camera` 컴포넌트(렌즈)와 GameObject `Transform`(배치) 의 분리와 동형.
     */
    class Camera
    {
    public:
        /// @brief 수직 시야각 (degree).
        float Fov = 60.0f;
        /// @brief 종횡비 (width/height). @ref SetAspect 가 갱신.
        float Aspect = 1.0f;
        /// @brief 가까운 클리핑 평면 (> 0).
        float NearPlane = 0.1f;
        /// @brief 먼 클리핑 평면 (> NearPlane).
        float FarPlane = 1000.0f;

        /// @brief 원근 투영 행렬 — 멤버 @c Aspect 사용.
        glm::mat4 GetProjMatrix() const
        {
            return glm::perspective(glm::radians(Fov), Aspect, NearPlane, FarPlane);
        }

        /// @brief 뷰포트 크기로 종횡비 갱신. height==0 (최소화) 이면 무시.
        void SetAspect(float width, float height)
        {
            if (height <= 0.0f)
                return;
            Aspect = width / height;
        }
    };
} // namespace SJH

#endif //__SJH_CAMERA_H__
```

- [ ] **Step 2: `context.cpp` Init — `mCamera` 대입 블록 제거**

`Init()` 의 카메라 대입 (현재 461~464행) 을 삭제한다:

```cpp
        mCamera = {
            .Pos = glm::vec3(0.0f, 2.5f, 8.0f),
            .EulerPitch = -20.f,
        };
```

(`mCamera` 는 클래스 기본값(Fov 60 / Near 0.1 / Far 1000)을 그대로 쓴다. 카메라 배치는
Step 4 의 Scene Graph 구성에서 노드로 설정.)

- [ ] **Step 3: `context.cpp` Init — 입력 바인딩 람다 재배선**

`Init()` 의 `// === Input 바인딩 ===` 블록 (현재 466~515행, `mKeyboard.BindKey` 부터
`BindLookHandler` 의 닫는 `});` 까지) 전체를 다음으로 교체:

```cpp
        // === Input 바인딩 ===
        // 1단 — 물리 키 -> 논리 액션 (InputMap 계층).
        mKeyboard.BindKey(GLFW_KEY_W, GameAction::MoveForward);
        mKeyboard.BindKey(GLFW_KEY_S, GameAction::MoveBackward);
        mKeyboard.BindKey(GLFW_KEY_D, GameAction::MoveRight);
        mKeyboard.BindKey(GLFW_KEY_A, GameAction::MoveLeft);
        mKeyboard.BindKey(GLFW_KEY_E, GameAction::MoveUp);
        mKeyboard.BindKey(GLFW_KEY_Q, GameAction::MoveDown);

        // 2단 — 논리 액션 -> 핸들러 (연속 이동). 카메라 노드를 증분 갱신.
        // front = 카메라 노드 월드 전방, right/up 은 수평 strafe 위해 world-up 과 외적.
        mKeyboard.BindHeldHandler(GameAction::MoveForward, [this] {
            mScene.At(SceneNodeId::Camera)
                .TranslateBy(kCameraSpeed * mScene.WorldForward(SceneNodeId::Camera));
        });
        mKeyboard.BindHeldHandler(GameAction::MoveBackward, [this] {
            mScene.At(SceneNodeId::Camera)
                .TranslateBy(-kCameraSpeed * mScene.WorldForward(SceneNodeId::Camera));
        });
        mKeyboard.BindHeldHandler(GameAction::MoveRight, [this] {
            const glm::vec3 front = mScene.WorldForward(SceneNodeId::Camera);
            const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -front));
            mScene.At(SceneNodeId::Camera).TranslateBy(kCameraSpeed * right);
        });
        mKeyboard.BindHeldHandler(GameAction::MoveLeft, [this] {
            const glm::vec3 front = mScene.WorldForward(SceneNodeId::Camera);
            const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -front));
            mScene.At(SceneNodeId::Camera).TranslateBy(-kCameraSpeed * right);
        });
        mKeyboard.BindHeldHandler(GameAction::MoveUp, [this] {
            const glm::vec3 front = mScene.WorldForward(SceneNodeId::Camera);
            const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -front));
            const glm::vec3 up = glm::normalize(glm::cross(-front, right));
            mScene.At(SceneNodeId::Camera).TranslateBy(kCameraSpeed * up);
        });
        mKeyboard.BindHeldHandler(GameAction::MoveDown, [this] {
            const glm::vec3 front = mScene.WorldForward(SceneNodeId::Camera);
            const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -front));
            const glm::vec3 up = glm::normalize(glm::cross(-front, right));
            mScene.At(SceneNodeId::Camera).TranslateBy(-kCameraSpeed * up);
        });

        // 마우스 우클릭 드래그 -> 카메라 노드 EulerRot (yaw=.y / pitch=.x) 갱신.
        mMouse.BindLookHandler([this](double dx, double dy) {
            glm::vec3 rot = mScene.At(SceneNodeId::Camera).Local().EulerRot;
            rot.y -= static_cast<float>(dx) * kCameraRotSpeed; // yaw
            rot.x -= static_cast<float>(dy) * kCameraRotSpeed; // pitch
            if (rot.y < 0.0f)
                rot.y += 360.0f;
            if (rot.y > 360.0f)
                rot.y -= 360.0f;
            if (rot.x > 89.0f)
                rot.x = 89.0f;
            if (rot.x < -89.0f)
                rot.x = -89.0f;
            mScene.At(SceneNodeId::Camera).SetEulerRot(rot);
        });
```

- [ ] **Step 4: `context.cpp` Init — Scene Graph 구성에 Camera 노드 추가**

`Init()` 의 Scene Graph 구성에서 `Window2` `SetLocal` 줄 다음에 추가:

```cpp
        // 카메라 — 위치 + (pitch, yaw, 0). view 행렬은 inverse(World(Camera)).
        mScene.SetLocal(SceneNodeId::Camera,
                        Transform{.Translate = {0.0f, 2.5f, 8.0f},
                                  .EulerRot = {-20.0f, 0.0f, 0.0f}});
```

`Attach` 그룹에서 `Window2` `Attach` 줄 다음에 추가:

```cpp
        mScene.Attach(SceneNodeId::Camera, SceneNodeId::Root);
```

- [ ] **Step 5: `context.cpp` Render — view 행렬·view pos 교체**

`Render()` 의 view 행렬 줄 (현재 211~213행) 을 교체:

```cpp
        // 카메라 노드의 월드 변환 역행렬 = view 행렬. proj 는 렌즈(mCamera).
        auto viewMat = glm::inverse(mScene.World(SceneNodeId::Camera));
        auto projMat = mCamera.GetProjMatrix();
```

`UNI_VIEW_POS` 전송 줄 (현재 246행) 을 교체:

```cpp
            Uniforms::SetVec3(*mProgram.get(), Const::UNI_VIEW_POS,
                              glm::vec3(mScene.World(SceneNodeId::Camera)[3]));
```

- [ ] **Step 6: `context.cpp` Render — flashlight 블록 (중간형) 교체**

`Render()` 의 flashlight 블록 (현재 257~261행) 을 교체 — `mCamera.Pos`/`GetFront()` 가
사라졌으므로 카메라 노드에서 취득 (스포트 구조체 필드는 Task 4 에서 제거되므로 본 Task 에선
유지):

```cpp
            if (mFlashLightMode)
            {
                mSpotLight.Pos = glm::vec3(mScene.World(SceneNodeId::Camera)[3]);
                mSpotLight.Direction = mScene.WorldForward(SceneNodeId::Camera);
            }
```

- [ ] **Step 7: `context.cpp` Render — ImGui 카메라 위젯 교체**

`Render()` 의 ImGui 카메라 위젯 + Reset 버튼 (현재 162~171행) 을 교체:

```cpp
            {
                Transform camT = mScene.At(SceneNodeId::Camera).Local();
                bool camChanged = false;
                camChanged |= ImGui::DragFloat3(Const::LBL_CAMERA_POS, glm::value_ptr(camT.Translate), 0.01f);
                camChanged |= ImGui::DragFloat(Const::LBL_CAMERA_YAW, &camT.EulerRot.y, 0.5f);
                camChanged |= ImGui::DragFloat(Const::LBL_CAMERA_PITCH, &camT.EulerRot.x, 0.5f, -89.0f, 89.0f);
                if (camChanged)
                    mScene.At(SceneNodeId::Camera).SetLocal(camT);
            }
            ImGui::Separator();
            if (ImGui::Button(Const::LBL_RESET_CAMERA))
            {
                mScene.At(SceneNodeId::Camera).SetLocal(Transform{.Translate = {0.0f, 0.0f, 3.0f}});
            }
```

- [ ] **Step 8: 빌드 — 통과 확인**

Run:
```bash
cmake --build build_Darwin
```
Expected: 빌드 성공. `mCamera.Pos` / `mCamera.GetFront` / `mCamera.EulerYaw` /
`mCamera.GetForwardViewMatrix` / `mCamera.CamUp` 잔존 참조가 있으면 컴파일 에러 — 위 Step
들이 모든 사용처를 덮어야 한다 (`mCamera.GetProjMatrix`/`SetAspect` 만 잔존, 정상).

- [ ] **Step 9: 회귀 — 실행 확인**

Run:
```bash
cmake --build build_Darwin --target tests
ctest --test-dir build_Darwin --output-on-failure
./build_Darwin/OpenGL-With-CMake
```
Expected: 전체 테스트 통과. 앱 실행 시 카메라 초기 시점(위에서 약간 내려다봄)·우클릭 드래그
회전·WASD/QE 이동이 리팩토링 전과 동일. 확인 후 창을 닫는다.

- [ ] **Step 10: 커밋**

```bash
git add src/object/camera.h src/context/context.cpp
git commit -m "$(printf '%s\n' \
  '[refactor] : Camera 를 투영 렌즈로 축소 — 배치는 Scene Graph 노드' '' \
  'Pos/EulerYaw/Pitch/GetFront/GetForwardViewMatrix 제거. view 행렬 =' \
  'inverse(World(Camera)). 입력 람다·Render·ImGui 가 카메라 노드를 갱신.' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Task 4: `Light` 축소 + `Uniforms::Set*Light` + Context 라이트 재배선

라이트 구조체에서 위치/방향을 제거하고, uniform 전송 함수가 노드에서 온 값을 받도록 한다.

**Files:**
- Modify: `src/object/light.h`
- Modify: `src/program/program_uniforms.h`
- Modify: `src/program/program_uniforms.cpp`
- Modify: `src/context/context.cpp`

- [ ] **Step 1: `src/object/light.h` — 위치/방향 멤버 제거**

`DirLight` 의 `Direction` 멤버 줄을 삭제:

```cpp
        /// @brief 평행광 방향 벡터 (world space). 셰이더 uniform `dirLight.direction`. 기본값은 아래-앞 방향.
        glm::vec3 Direction{glm::vec3(-0.2f, -1.0f, -0.3f)};
```

`PointLight` 의 `Pos` 멤버 줄을 삭제:

```cpp
        /// @brief 점 광원 월드 좌표. 셰이더 uniform `pointLights[i].position`. ImGui DragFloat3 위젯이 갱신.
        glm::vec3 Pos{glm::vec3(3.0f, 3.0f, 3.0f)};
```

`SpotLight` 의 `Pos` 와 `Direction` 멤버 줄을 삭제:

```cpp
        /// @brief 광원 월드 좌표. 셰이더 uniform `light.position`. ImGui DragFloat3 위젯이 갱신.
        glm::vec3 Pos{glm::vec3(3.0f, 3.0f, 3.0f)};

        /// @brief 스포트 콘 축 방향 (world space, 정규화 권장). 셰이더 uniform `light.direction`.
        /// @details `normalize(-mDirection)` 과 lightDir 의 dot 으로 `theta` 계산 -> cutoff 비교.
        glm::vec3 Direction{glm::vec3(0.0f, -1.0f, 0.0f)};
```

> `DirLight`/`PointLight`/`SpotLight` 의 색상 3항·`Distance`·cutoff 멤버와 자유 함수
> `GetAttenuationCoeff` 는 유지. 미사용 `Light` 베이스 클래스는 손대지 않는다.

- [ ] **Step 2: `src/program/program_uniforms.h` — `Set*Light` 시그니처 변경**

`SetDirLight`/`SetPointLight`/`SetSpotLight` 선언 3개를 교체:

```cpp
        /// @brief DirLight -> `<prefix>.{direction,ambient,diffuse,specular}` 4 uniform 전송.
        /// @param worldDir 라이트 노드의 월드 전방 벡터 (방향).
        void SetDirLight(const Program &prog, const char *prefix,
                         const DirLight &light, const glm::vec3 &worldDir);

        /// @brief PointLight -> `<prefix>.{position,attenuation,ambient,diffuse,specular}` 5 uniform 전송.
        /// @param worldPos 라이트 노드의 월드 위치.
        void SetPointLight(const Program &prog, const char *prefix,
                           const PointLight &light, const glm::vec3 &worldPos);

        /// @brief SpotLight -> 8 uniform 전송
        ///        (`<prefix>.{position,direction,cutoff,outerCutoff,attenuation,ambient,diffuse,specular}`).
        /// @param worldPos 라이트 노드의 월드 위치. @param worldDir 라이트 노드의 월드 전방.
        void SetSpotLight(const Program &prog, const char *prefix,
                          const SpotLight &light, const glm::vec3 &worldPos, const glm::vec3 &worldDir);
```

- [ ] **Step 3: `src/program/program_uniforms.cpp` — `Set*Light` 본문 교체**

`SetDirLight`/`SetPointLight`/`SetSpotLight` 정의 3개 (현재 188~219행) 를 교체:

```cpp
    void SetDirLight(const Program &prog, const char *prefix,
                     const DirLight &light, const glm::vec3 &worldDir)
    {
        const std::string base = prefix;
        SetVec3(prog, (base + Const::SFX_DIRECTION).c_str(), worldDir);
        SetVec3(prog, (base + Const::SFX_AMBIENT).c_str(),   light.Ambient);
        SetVec3(prog, (base + Const::SFX_DIFFUSE).c_str(),   light.Diffuse);
        SetVec3(prog, (base + Const::SFX_SPECULAR).c_str(),  light.Specular);
    }

    void SetPointLight(const Program &prog, const char *prefix,
                       const PointLight &light, const glm::vec3 &worldPos)
    {
        const std::string base = prefix;
        SetVec3(prog, (base + Const::SFX_POSITION).c_str(),    worldPos);
        SetVec3(prog, (base + Const::SFX_ATTENUATION).c_str(), GetAttenuationCoeff(light.Distance));
        SetVec3(prog, (base + Const::SFX_AMBIENT).c_str(),     light.Ambient);
        SetVec3(prog, (base + Const::SFX_DIFFUSE).c_str(),     light.Diffuse);
        SetVec3(prog, (base + Const::SFX_SPECULAR).c_str(),    light.Specular);
    }

    void SetSpotLight(const Program &prog, const char *prefix,
                      const SpotLight &light, const glm::vec3 &worldPos, const glm::vec3 &worldDir)
    {
        const std::string base = prefix;
        SetVec3 (prog, (base + Const::SFX_POSITION).c_str(),     worldPos);
        SetVec3 (prog, (base + Const::SFX_DIRECTION).c_str(),    worldDir);
        SetFloat(prog, (base + Const::SFX_CUTOFF).c_str(),       cosf(glm::radians(light.CutoffAngleDeg)));
        SetFloat(prog, (base + Const::SFX_OUTER_CUTOFF).c_str(), cosf(glm::radians(light.OuterCutoffAngleDeg)));
        SetVec3 (prog, (base + Const::SFX_ATTENUATION).c_str(),  GetAttenuationCoeff(light.Distance));
        SetVec3 (prog, (base + Const::SFX_AMBIENT).c_str(),      light.Ambient);
        SetVec3 (prog, (base + Const::SFX_DIFFUSE).c_str(),      light.Diffuse);
        SetVec3 (prog, (base + Const::SFX_SPECULAR).c_str(),     light.Specular);
    }
```

- [ ] **Step 4: `context.cpp` Init — 라이트 구조체 초기화에서 위치/방향 제거**

`Init()` 의 라이트 대입 블록 (현재 434~459행) 을 교체:

```cpp
        mDirLight = {
            .Ambient = glm::vec3(1.0f, 1.0f, 1.0f),
            .Diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
            .Specular = glm::vec3(1.0f, 1.0f, 1.0f)};

        mPointLights[0] = {.Distance = 50.0f,
                           .Ambient = glm::vec3(0.05f, 0.05f, 0.05f),
                           .Diffuse = glm::vec3(0.8f, 0.4f, 0.2f),
                           .Specular = glm::vec3(1.0f, 1.0f, 1.0f)};

        mPointLights[1] = {.Distance = 50.0f,
                           .Ambient = glm::vec3(0.05f, 0.05f, 0.05f),
                           .Diffuse = glm::vec3(0.2f, 0.4f, 0.8f),
                           .Specular = glm::vec3(1.0f, 1.0f, 1.0f)};

        mSpotLight = {.CutoffAngleDeg = 5.0f,
                      .OuterCutoffAngleDeg = 120.0f,
                      .Distance = 128.0f,
                      .Ambient = glm::vec3(0.0f, 0.0f, 0.0f),
                      .Diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
                      .Specular = glm::vec3(1.0f, 1.0f, 1.0f)};
```

- [ ] **Step 5: `context.cpp` Init — Scene Graph 의 라이트 노드 변환 완성 + DirLight 추가**

`Init()` 의 Scene Graph 구성에서 `PointLight0/1`/`SpotLight` `SetLocal` 3줄
(Task 2 Step 3 에서 리네임한 줄) 을 *위치 포함* 으로 교체:

```cpp
        // 라이트 노드 — 위치(Translate) + 마커 큐브 크기(Scale 0.1). 방향은 EulerRot.
        mScene.SetLocal(SceneNodeId::DirLight,
                        Transform{.EulerRot = {-90.0f, 0.0f, 0.0f}}); // forward = (0,-1,0)
        mScene.SetLocal(SceneNodeId::PointLight0,
                        Transform{.Translate = {1.2f, 1.0f, 1.0f}, .Scale = {0.1f, 0.1f, 0.1f}});
        mScene.SetLocal(SceneNodeId::PointLight1,
                        Transform{.Translate = {-1.2f, 1.0f, -1.0f}, .Scale = {0.1f, 0.1f, 0.1f}});
        mScene.SetLocal(SceneNodeId::SpotLight,
                        Transform{.Translate = {1.0f, 4.0f, 4.0f},
                                  .EulerRot = {-90.0f, 0.0f, 0.0f}, // forward = (0,-1,0)
                                  .Scale = {0.1f, 0.1f, 0.1f}});
```

`Attach` 그룹의 `PointLight0/1`/`SpotLight` `Attach` 3줄 (Task 2 Step 3 에서 리네임) 다음에
`DirLight` 부착을 추가:

```cpp
        mScene.Attach(SceneNodeId::DirLight, SceneNodeId::Root);
```

- [ ] **Step 6: `context.cpp` Render — 마커 드로우 블록 교체**

`Render()` 의 마커 블록 (현재 217~242행, `// 두 점광원...` 주석부터 닫는 `}` 까지) 을 교체 —
per-frame `SetTranslate` 복사 제거, 라이트 노드 `World()` 에 직접 그림:

```cpp
            // 두 점광원 + 스포트라이트 = 총 3개 마커 큐브. 각자 자기 diffuse 색으로 출력.
            const glm::vec3 markerColors[3] = {
                mPointLights[0].Diffuse,
                mPointLights[1].Diffuse,
                mSpotLight.Diffuse,
            };
            const SceneNodeId markerIds[3] = {
                SceneNodeId::PointLight0, SceneNodeId::PointLight1, SceneNodeId::SpotLight};
            for (int i = 0; i < 3; ++i)
            {
                if (mFlashLightMode && i >= 2)
                    continue;
                Uniforms::SetVec4(*mSimpleProgram.get(), Const::UNI_BASE_COLOR,
                                  glm::vec4(markerColors[i], 1.0f));
                Uniforms::SetMat4(*mSimpleProgram.get(), Const::UNI_TRANSFORM_MAT,
                                  projMat * viewMat * mScene.World(markerIds[i]));
                mBox->Draw();
            }
```

- [ ] **Step 7: `context.cpp` Render — flashlight + 라이트 uniform 전송 교체**

`Render()` 의 flashlight 블록 (Task 3 Step 6 에서 작성한 4줄) 을 교체 — 스포트 노드가
카메라 노드를 추종 (마커 스케일 0.1 은 보존):

```cpp
            if (mFlashLightMode)
            {
                Transform spotT = mScene.At(SceneNodeId::Camera).Local();
                spotT.Scale = glm::vec3(0.1f, 0.1f, 0.1f); // 마커 큐브 크기 유지
                mScene.At(SceneNodeId::SpotLight).SetLocal(spotT);
            }
```

`SetSpotLight`/`SetDirLight` 전송 (현재 262~264행) 을 교체:

```cpp
            Uniforms::SetSpotLight(*mProgram.get(), Const::UNI_SPOT_LIGHT, mSpotLight,
                                   glm::vec3(mScene.World(SceneNodeId::SpotLight)[3]),
                                   mScene.WorldForward(SceneNodeId::SpotLight));
            // --- DirLight 1개 (평행광 / 거리감쇠 없음) ---
            Uniforms::SetDirLight(*mProgram, Const::UNI_DIR_LIGHT, mDirLight,
                                  mScene.WorldForward(SceneNodeId::DirLight));
```

PointLight 전송 루프 (현재 267~271행) 를 교체:

```cpp
            // --- PointLight 2개 (배열, 거리감쇠는 helper 가 mDistance 로 내부 도출) ---
            const SceneNodeId pointLightIds[2] = {
                SceneNodeId::PointLight0, SceneNodeId::PointLight1};
            for (int i = 0; i < 2; ++i)
            {
                const std::string base = Const::UNI_POINT_LIGHTS_PREFIX + std::to_string(i) + Const::STR_INDEX_CLOSE;
                Uniforms::SetPointLight(*mProgram, base.c_str(), mPointLights[i],
                                        glm::vec3(mScene.World(pointLightIds[i])[3]));
            }
```

SpotLight 재전송 블록 (현재 273~280행) 의 중복 `SetSpotLight` 호출을 교체 — 이 블록은
`UNI_TRANSFORM_MAT` 만 설정하면 되므로 `SetSpotLight` 줄을 제거:

```cpp
            // --- SpotLight uniform 은 위에서 전송됨 — 본 블록은 transform uniform 만 ---
            {
                auto modelTransform = glm::mat4(1.0f);
                auto transform = projMat * viewMat * modelTransform;
                Uniforms::SetMat4(*mProgram.get(), Const::UNI_TRANSFORM_MAT, transform);
                Uniforms::SetMat4(*mProgram.get(), Const::UNI_MODEL_TRANSFORM_MAT, modelTransform);
            }
```

- [ ] **Step 8: `context.cpp` Render — ImGui 라이트 위젯 교체**

`Render()` 의 ImGui DirLight 위젯 중 방향 줄 (현재 115행) 을 교체:

```cpp
                {
                    Transform t = mScene.At(SceneNodeId::DirLight).Local();
                    if (ImGui::DragFloat3(Const::LBL_DIR_DIRECTION, glm::value_ptr(t.EulerRot), 0.5f))
                        mScene.At(SceneNodeId::DirLight).SetLocal(t);
                }
```

PointLight 위젯의 위치 줄 (현재 128행) 을 교체:

```cpp
                    {
                        const SceneNodeId pid = (i == 0) ? SceneNodeId::PointLight0 : SceneNodeId::PointLight1;
                        Transform t = mScene.At(pid).Local();
                        if (ImGui::DragFloat3(Const::LBL_P_POSITION, glm::value_ptr(t.Translate), 0.01f))
                            mScene.At(pid).SetLocal(t);
                    }
```

SpotLight 위젯의 위치·방향 줄 (현재 140~141행) 을 교체:

```cpp
                {
                    Transform t = mScene.At(SceneNodeId::SpotLight).Local();
                    bool spotChanged = false;
                    spotChanged |= ImGui::DragFloat3(Const::LBL_S_POSITION, glm::value_ptr(t.Translate), 0.01f);
                    spotChanged |= ImGui::DragFloat3(Const::LBL_S_DIRECTION, glm::value_ptr(t.EulerRot), 0.5f);
                    if (spotChanged)
                        mScene.At(SceneNodeId::SpotLight).SetLocal(t);
                }
```

> ImGui 방향 위젯(`LBL_DIR_DIRECTION`/`LBL_S_DIRECTION`)은 이제 노드 EulerRot(degree)을
> 편집한다 — 라벨 문자열은 그대로 둔다 (디버그 UI, 추후 리라벨은 선택).

- [ ] **Step 9: 빌드 — 통과 확인**

Run:
```bash
cmake --build build_Darwin
```
Expected: 빌드 성공. `mDirLight.Direction` / `mPointLights[i].Pos` / `mSpotLight.Pos` /
`mSpotLight.Direction` 잔존 참조가 있으면 컴파일 에러 — 위 Step 들이 모든 사용처를 덮어야 한다.

- [ ] **Step 10: 회귀 — 실행 확인**

Run:
```bash
cmake --build build_Darwin --target tests
ctest --test-dir build_Darwin --output-on-failure
./build_Darwin/OpenGL-With-CMake
```
Expected: 전체 테스트 통과. 앱 실행 시 — 라이팅(평행광/점광원 2개/스포트), 광원 마커 큐브
위치·색, flashlight 모드 토글이 리팩토링 전과 동일. 확인 후 창을 닫는다.

- [ ] **Step 11: 커밋**

```bash
git add src/object/light.h src/program/program_uniforms.h src/program/program_uniforms.cpp \
        src/context/context.cpp
git commit -m "$(printf '%s\n' \
  '[refactor] : Light 위치/방향을 Scene Graph 노드로 — 구조체는 색상/감쇠만' '' \
  'DirLight/PointLight/SpotLight 에서 Pos/Direction 제거. Uniforms::Set*Light 가' \
  '위치/방향을 노드에서 명시 인자로 받음. 마커=라이트 노드, flashlight=카메라 노드 추종.' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Task 5: 문서 갱신

**Files:**
- Modify: `.claude/MEMORY.md`, `.claude/architecture.md` (git 비추적 — 작업 트리만)
- Modify: `doc/pages/00-mainpage.md` (`doxygen-class-graph` skill 경유)

- [ ] **Step 1: `.claude/MEMORY.md` / `.claude/architecture.md` 갱신**

`SJH::object` 모듈 항목에서 `Camera`(렌즈 전용), `Light`(위치/방향 제거 — 노드 소유) 변경을
반영. `SceneNode` 에 `WorldForward`/`TranslateBy` 추가 언급. `architecture.md` 의 `SJH::object`
행에 설계 출처 [doc/design/2026-05-19-camera-light-scene-graph-design.md](../doc/design/2026-05-19-camera-light-scene-graph-design.md) 링크.

- [ ] **Step 2: doxygen 클래스 그래프 갱신**

`doxygen-class-graph` skill 을 호출해 `doc/pages/00-mainpage.md` 의 클래스 의존 그래프를
갱신한다 — `Camera` 가 `SceneGraph`/`SceneNode` 와 더 이상 위치 결합이 없음(렌즈 전용),
라이트 위치/방향이 노드 소유임을 반영.

- [ ] **Step 3: 커밋**

```bash
git add doc/pages/00-mainpage.md
git commit -m "$(printf '%s\n' \
  '[doc] : Camera/Light Scene Graph 통합 — doxygen 클래스 그래프 갱신' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Self-Review 결과

- **Spec 커버리지:** 설계 §4(SceneNode/SceneGraph 보강)=Task1, §3(SceneNodeId)=Task2,
  §5(Camera 축소)=Task3, §6(Light 축소)+§7(Set*Light)=Task4, §8(Context 재배선)=Task3·4,
  §9(테스트)=Task1, §13(Phase 분해)=Task1-5 와 1:1. 누락 없음.
- **Placeholder:** 없음 — 모든 코드 블록은 실제 코드, 수정 지점은 현재 행 번호 + 교체 전 코드
  특정으로 명시.
- **타입 일관성:** `SceneNode::{WorldForward,TranslateBy}`, `SceneGraph<TId>::WorldForward`,
  `Camera::{GetProjMatrix,SetAspect}`, `Uniforms::SetDirLight/SetPointLight/SetSpotLight` 의
  새 시그니처(`worldDir`/`worldPos`), `SceneNodeId::{Camera,DirLight,PointLight0,PointLight1,
  SpotLight}` — Task 간 일치 확인 완료.
- **빌드 green 불변식:** Task1(소비자 무변경) · Task2(순수 리네임+enum 추가, 동작 불변) ·
  Task3(Camera 사용처 전부 교체 — 컴파일러가 누락 검출) · Task4(Light 사용처 전부 교체).
  Task3 의 flashlight 블록은 중간형(스포트 구조체 필드 유지)으로 green 유지 후 Task4 가 최종화.
