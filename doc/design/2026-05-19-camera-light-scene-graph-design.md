# Camera + Light → Scene Graph 통합 — 설계

- **날짜**: 2026-05-19
- **대상 모듈**: `src/object/` (camera / light / scene_node / scene_graph), `src/program/`, `src/context/`
- **상태**: 승인됨 (구현 계획 수립 단계)
- **선행**: [2026-05-19-transform-scene-graph-design.md](2026-05-19-transform-scene-graph-design.md) (SceneNode/SceneGraph),
  [2026-05-19-input-module-design.md](2026-05-19-input-module-design.md) (입력 모듈 — 입력 콜백이 본 설계에서 노드를 갱신).

## 1. 동기 & 목표

`Camera` 와 라이트(`DirLight`/`PointLight`/`SpotLight`)는 위치·방향을 *자기 멤버* 로 들고 있어
Scene Graph 밖에 있다. 그 결과:

- 라이트 마커 큐브 노드(`PointMarker0/1`/`SpotMarker`)가 매 프레임 라이트 `Pos` 를 *복사* —
  노드가 라이트의 분신일 뿐, 단일 소유자가 아님.
- 카메라 위치·회전이 씬 그래프와 무관 — 계층·부모 추종(예: flashlight) 을 표현 못 함.
- 위치/방향이 두 표현(클래스 멤버 + (마커)노드)으로 갈림.

**목표**: `Camera` 와 모든 라이트를 Scene Graph 노드로 편입한다. 노드의 `Transform` 이
위치·방향의 **단일 소유자**가 되고, `Camera`/`Light` 클래스는 *비-변환* 데이터(렌즈 파라미터,
Phong 색상, 감쇠, cutoff)만 보유한다. (선행 결정: 위치 + 방향 모두 통합, 입력 콜백이 카메라
노드를 갱신, `DirLight` 도 rotation-only 노드, Camera·Light 를 한 사이클로 통합.)

## 2. 조사 근거 (Context7 — 선행 사이클)

- **Unity** — `Camera` 는 *컴포넌트*(Fov/clip/projection), 배치는 GameObject 의 `Transform`.
  본 설계의 "`Camera`=렌즈, 노드=배치" 분리와 동형.
- **Unity/Unreal** — directional light 도 `Transform`(또는 `USceneComponent`)을 가지며 *회전만*
  의미. 본 설계의 `DirLight` rotation-only 노드와 일치.
- 라이트/카메라의 *방향* = 노드의 forward(회전된 -Z) — 엔진 공통.

## 3. `SceneNodeId` 변경 (`src/context/scene_node_id.h`)

마커 노드를 라이트 노드로 승격(리네임)하고 Camera/DirLight 를 추가한다:

```cpp
enum class SceneNodeId : std::size_t
{
    Root = 0,
    Plane, Box1, Box2, Outline,
    Windows, Window0, Window1, Window2,
    Camera,
    DirLight, PointLight0, PointLight1, SpotLight,
    Count
};
```

- `PointMarker0/1` → `PointLight0/1`, `SpotMarker` → `SpotLight`. 마커 큐브는 라이트의
  *시각화*일 뿐 — 노드는 라이트 엔티티 자체.
- `Camera`, `DirLight`, `PointLight0/1`, `SpotLight` 모두 `Root` 자식 (Render 의 `Attach` 에서 구성).

## 4. `SceneNode` / `SceneGraph` 보강

### `SceneNode` (`src/object/scene_node.{h,cpp}`)

```cpp
/// @brief 노드의 월드 전방 단위 벡터 — 회전된 -Z. 카메라 front·라이트 방향에 사용.
glm::vec3 WorldForward() const;          // normalize(vec3(World() * vec4(0,0,-1,0)))

/// @brief 로컬 Translate 를 @p delta 만큼 증분 — 자신+자손 dirty.
void TranslateBy(const glm::vec3 &delta);
```

- `WorldForward` — 카메라 이동 front + 라이트(spot/dir) 방향을 일원화. `World()` 에서 직접
  추출하므로 dirty 캐싱 재사용.
- `TranslateBy` — 카메라 WASD 증분 이동용. 내부적으로 `mLocal.Translate += delta` 후
  `MarkSubtreeDirty()`.

### `SceneGraph<TId>` (`src/object/scene_graph.h`)

```cpp
/// @brief @p id 노드의 월드 전방 벡터.
glm::vec3 WorldForward(TId id) const { return mNodes[Index(id)].WorldForward(); }
```

`World(TId)` 와 짝을 이루는 위임 메서드.

## 5. `Camera` — 렌즈 전용으로 축소 (`src/object/camera.h`)

위치·방향이 노드로 가므로 `Camera` 는 *투영 렌즈* 만 남긴다 (Unity `Camera` 컴포넌트와 동형):

```cpp
class Camera
{
public:
    float Fov       = 60.0f;   ///< 수직 시야각 (degree)
    float Aspect    = 1.0f;    ///< 종횡비 — SetAspect 가 갱신
    float NearPlane = 0.1f;
    float FarPlane  = 1000.0f;

    glm::mat4 GetProjMatrix() const
    {
        return glm::perspective(glm::radians(Fov), Aspect, NearPlane, FarPlane);
    }
    void SetAspect(float width, float height)
    {
        if (height <= 0.0f) return;
        Aspect = width / height;
    }
};
```

- **제거**: `Pos`, `Target`, `CamUp`, `EulerYaw`, `EulerPitch`, `GetFront()`,
  `GetForwardViewMatrix()`, `GetLookAtViewMatrix()`.
- view 행렬은 `Camera` 가 아니라 카메라 *노드* 에서 산출 (§8).
- 카메라 배치(위치+yaw/pitch)는 `SceneNodeId::Camera` 노드의 `Transform` 이 소유 —
  `EulerRot = (pitch, yaw, 0)`.

## 6. `Light` 클래스 — 위치/방향 제거 (`src/object/light.h`)

| 클래스 | 제거 | 유지 |
|---|---|---|
| `PointLight` | `Pos` | `Distance`, `Ambient`/`Diffuse`/`Specular` |
| `SpotLight` | `Pos`, `Direction` | `CutoffAngleDeg`, `OuterCutoffAngleDeg`, `Distance`, 색상 3항 |
| `DirLight` | `Direction` | `Ambient`/`Diffuse`/`Specular` |

- 위치 = 라이트 노드의 `Transform::Translate`. 방향 = 라이트 노드의 `WorldForward()`.
- `GetAttenuationCoeff(float)` 자유 함수는 무변경.
- 미사용 `Light` 클래스(light.h 의 점광원 베이스)는 본 설계 범위 밖 — 손대지 않는다.

## 7. `Uniforms::Set*Light` 시그니처 변경 (`src/program/program_uniforms.{h,cpp}`)

라이트 구조체가 위치/방향을 잃으므로, 셰이더 uniform 전송 함수가 노드에서 온 값을 명시로 받는다:

```cpp
void SetPointLight(const Program &program, const char *name,
                   const PointLight &light, const glm::vec3 &worldPos);
void SetSpotLight (const Program &program, const char *name,
                   const SpotLight &light, const glm::vec3 &worldPos, const glm::vec3 &worldDir);
void SetDirLight  (const Program &program, const char *name,
                   const DirLight &light, const glm::vec3 &worldDir);
```

- 함수 본문: 색상/감쇠/cutoff 는 `light` 구조체에서, 위치/방향은 새 인자에서 읽어 uniform 으로 push.
- 호출지점(`Context::Render`)이 `worldPos`/`worldDir` 를 `mScene` 노드에서 취득해 전달.

## 8. Context 재배선 (`src/context/context.{h,cpp}`)

### 멤버
- `Camera mCamera;` (렌즈), `DirLight mDirLight;` / `PointLight mPointLights[2];` /
  `SpotLight mSpotLight;` (색상 등) — 유지. 위치/방향만 노드로.
- `SceneGraph<SceneNodeId> mScene;` — 기존 멤버, 노드 추가.

### `Init()`
- 카메라·라이트의 위치/방향을 `mScene.SetLocal(...)` 로 설정. 예:
  `mScene.SetLocal(SceneNodeId::Camera, Transform{.Translate={0,2.5,8}, .EulerRot={-20,0,0}})`,
  `mScene.SetLocal(SceneNodeId::DirLight, Transform{.EulerRot={-90,0,0}})` (forward = (0,-1,0)),
  `mScene.SetLocal(SceneNodeId::PointLight0, Transform{.Translate={1.2,1,1}})` 등.
- 색상/감쇠/cutoff 는 구조체 지정 초기화로 (Pos/Direction 필드만 빠짐).
- `mScene.Attach(...)` 로 Camera/DirLight/라이트 4개를 `Root` 자식으로 등록.

### `Render()`
- **view 행렬**: `auto viewMat = glm::inverse(mScene.World(SceneNodeId::Camera));`
  (기존 `mCamera.GetForwardViewMatrix()` 와 수학적 동일 — roll 없는 FPS 카메라에서
  `inverse(T·Ry·Rx)` == `lookAt(pos, pos+front, worldUp)`).
- **proj 행렬**: `mCamera.GetProjMatrix()` — 무변경.
- **`UNI_VIEW_POS`**: `glm::vec3(mScene.World(SceneNodeId::Camera)[3])` (월드 위치 = 변환 행렬 4열).
- **라이트 uniform**: 각 라이트의 위치/방향을 해당 `SceneNodeId` 노드에서 취득해 전달 —
  PointLight 0/1 은 `SceneNodeId::PointLight0`/`PointLight1` 노드의 `World()` 위치를
  `SetPointLight` 에, SpotLight 은 노드의 `World()` 위치 + `WorldForward()` 방향을
  `SetSpotLight` 에, DirLight 은 노드의 `WorldForward()` 방향을 `SetDirLight` 에.
- **마커 큐브**: `mScene.At(markerId).SetTranslate(light.Pos)` per-frame 복사 **제거** —
  큐브를 `projMat * viewMat * mScene.World(PointLight0)` 등에 직접 그림.
- **flashlight 모드**: `mSpotLight.Pos = mCamera.Pos; mSpotLight.Direction = mCamera.GetFront();`
  → `mScene.At(SpotLight).SetLocal(mScene.At(Camera).Local());` (스포트 노드가 카메라 노드 추종).
- **ImGui** — 카메라/라이트 위치·방향 위젯은 temp-edit 패턴:
  ```cpp
  Transform t = mScene.At(id).Local();
  if (ImGui::DragFloat3(label, glm::value_ptr(t.Translate))) mScene.At(id).SetLocal(t);
  ```
  방향 위젯(spot/dir light)은 `t.EulerRot` 를 편집 (degree). "Reset Camera" 버튼도 카메라
  노드 `SetLocal` 로.

### 입력 재배선 (`Init()` 의 바인딩 람다)
- 이동 람다(MoveForward 등): `mScene.WorldForward(Camera)` 로 front 산출, right/up 은
  `cross(worldUp, -front)` / `cross(-front, right)` (worldUp = `{0,1,0}` 리터럴 — 수평 strafe
  유지). `mScene.At(Camera).TranslateBy(kCameraSpeed * dir)`.
- look 람다: 카메라 노드 `Local().EulerRot` 의 yaw(`.y`)·pitch(`.x`) 를 dx/dy 로 갱신,
  yaw `[0,360)` 정규화 / pitch `[-89,89]` 클램프 후 `SetEulerRot`.

## 9. 테스트

- `test/test_scene_node.cpp` += `WorldForward`(EulerRot → 전방 벡터: 기본 →(0,0,-1),
  yaw 90° → (-1,0,0) 등) · `TranslateBy`(증분 누적) 케이스.
- `test/test_scene_graph.cpp` += `WorldForward(TId)` 위임 케이스.
- `Camera`/`Light` 멤버 제거는 컴파일러가 회귀를 잡는다 — 제거된 멤버를 참조하는 코드가
  남으면 빌드 에러.
- Context 시각 회귀: view 행렬·라이팅 결과가 통합 전후 수학적으로 동일 — 앱 실행 육안 확인.

## 10. 영향 파일

| 분류 | 파일 |
|---|---|
| 수정 | `src/object/scene_node.h`, `src/object/scene_node.cpp` — `WorldForward` / `TranslateBy` |
| 수정 | `src/object/scene_graph.h` — `WorldForward(TId)` |
| 수정 | `src/context/scene_node_id.h` — Camera/DirLight 추가, 마커 리네임 |
| 수정 | `src/object/camera.h` — 렌즈 전용 축소 |
| 수정 | `src/object/light.h` — Pos/Direction 제거 |
| 수정 | `src/program/program_uniforms.h`, `src/program/program_uniforms.cpp` — `Set*Light` 시그니처 |
| 수정 | `src/context/context.h`, `src/context/context.cpp` — Init/Render/ImGui/입력 재배선 |
| 수정 | `test/test_scene_node.cpp`, `test/test_scene_graph.cpp` |
| 문서 | `.claude/MEMORY.md`, `.claude/architecture.md`, `doc/pages/00-mainpage.md` (doxygen 그래프) |

가장 큰 작업 지점은 `context.cpp` 의 `Init`·`Render`(ImGui 블록 포함)·입력 람다.

## 11. 범위 밖 (Non-Goals)

- 미사용 `Light` 베이스 클래스 정리 — 별개 사안.
- 카메라 look-at 모드 (`Target` 기반) — 제거. 필요 시 노드에 `LookAt` 추가는 후속.
- 라이트/카메라 노드를 `Root` 외 다른 노드의 자식으로 중첩 — 현재 전부 `Root` 직속.
- `Uniforms::Set*Light` 외 다른 uniform 경로 변경.
- 카메라 roll (Z축 회전) — yaw/pitch 만, EulerRot.z 는 항상 0.

## 12. 검토했으나 채택하지 않은 대안

| 대안 | 기각 사유 |
|---|---|
| 라이트 구조체에 `Pos` 유지 + 노드에서 매 프레임 sync | "구조체가 노드를 mirror" — 현재의 "노드가 구조체를 mirror" 를 뒤집을 뿐, 단일 소유자 부재는 그대로 |
| `Camera` 가 노드를 멤버로 보유 | `Camera`(렌즈)와 노드(배치)는 다른 수명·소유 — `mScene` 가 노드 소유, Camera 는 렌즈만 (Unity 분리와 동형) |
| `DirLight` 를 노드에서 제외 (Direction 필드 유지) | 라이트 처리가 비일관 (point/spot 은 노드, dir 은 아님) — 사용자 결정으로 rotation-only 노드 채택 |
| `Camera` 클래스 완전 삭제, Fov 등을 Context 직접 보유 | 투영 파라미터 묶음은 응집된 단위 — Unity `Camera` 컴포넌트처럼 렌즈 객체로 유지 |

## 13. 구현 Phase 분해 (구현 계획서에서 상세화)

빌드가 매 단계 green 을 유지하도록:

1. **`SceneNode`/`SceneGraph` 보강** — `WorldForward` / `TranslateBy` + 테스트. 소비자 없이 독립.
2. **`SceneNodeId` 확장** — Camera/DirLight 추가, 마커 리네임. `context.cpp` 의 기존
   `PointMarker*`/`SpotMarker` 참조를 새 이름으로 (mechanical, 동작 불변).
3. **`Camera` 축소** — 렌즈 전용. `context.cpp` 의 카메라 위치/방향/뷰 사용처를 노드 기반으로
   교체 (Init·Render·입력 람다). 앱 실행 회귀 확인.
4. **`Light` 축소 + `Set*Light` 시그니처** — light.h·program_uniforms 변경, `context.cpp` 의
   라이트 Init·uniform 전송·마커·flashlight·ImGui 재배선. 앱 실행 회귀 확인.
5. **문서 갱신** — MEMORY / architecture / doxygen 그래프.
