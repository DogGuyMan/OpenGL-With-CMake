# 입력 모듈 (`SJH::input`) — 설계

- **날짜**: 2026-05-19
- **대상 모듈**: `src/input/` (신규), `src/context/`, `src/object/camera.h`
- **상태**: 승인됨 (구현 계획 수립 단계)
- **설계 출처**: 본 문서. Context7 엔진 입력 API 조사 (Unity New Input System / Unreal Enhanced Input / Godot / Cocos2d-x).

## 1. 동기 & 목표

`Context`가 GLFW 입력을 직접 처리한다 — `ProcessInput`(WASD/QE 폴링), `MouseMove`/`MouseButton`(yaw/pitch·드래그). 문제:

- 입력 폴링·델타 계산·카메라 갱신이 `Context`에 뒤섞임 — 재사용·테스트 불가.
- 키 바인딩이 코드에 하드코딩 — 재매핑하려면 `Context` 수정.
- `Camera::IsCamControl` — 입력 상태가 `Camera`에 얹혀 있음 (camera.h 스스로 "❌ 입력 처리"라 명시하면서도).

**목표**: 입력을 독립 모듈 `SJH::input`으로 추출한다. SSU `inputs` 모듈의 콜백 바인딩 패턴 +
엔진 모범의 **논리 액션 계층**을 적용하되, 입력 모듈이 게임플레이 어휘를 모르도록 **액션 타입에
제네릭**하게 만든다.

> 본 설계는 입력 모듈 *추출*만 다룬다. Camera/Light 를 Scene Graph 노드로 편입하는 작업은
> 후속 사이클 — 입력 모듈의 콜백이 `mCamera` 대신 카메라 *노드*를 갱신하도록 람다만 교체하면
> 되는 구조로 본 설계가 길을 연다.

## 2. 조사 근거 (Context7)

### 2.1 4개 엔진 입력 처리 — 공통 모범

| 엔진 | 핵심 추상 | press/release/held | 어휘 소유 |
|---|---|---|---|
| Unity New Input System | `InputAction` ← `InputBinding` ← 물리 control | `started`/`performed`/`canceled` + `ReadValue<T>()` | `InputActionAsset` (데이터) |
| Unreal Enhanced Input | `InputAction` + `InputMappingContext` + Trigger/Modifier | `BindAction(action, ETriggerEvent, …)` | `InputAction` 에셋 (데이터) |
| Godot | `InputMap` 명명 액션 | `_input(event)` 이산 + `Input.is_action_pressed()` 연속 | 프로젝트 설정 문자열 (데이터) |
| Cocos2d-x | `EventDispatcher` + `EventListener` | `onKeyPressed`/`onKeyReleased` | — |

**모범 패턴 3가지:**

1. **논리 액션 추상화** — "W 키"가 아니라 "MoveForward 액션"을 코드가 다룬다. 물리↔논리 매핑은 데이터.
2. **이벤트 + 폴링 혼용** — 이산 입력은 이벤트, 연속 입력은 폴링. Godot가 가장 명시적.
3. **액션 어휘를 입력 시스템이 소유하지 않는다** — Godot 문자열·Unity/Unreal 에셋. 게임이 소유.

### 2.2 SSU `inputs` 모듈 대비

SSU `KeybaordInput`(키→`std::function` 직접 바인딩) / `MouseInput`(드래그 상태 + `dragAction(dx,dy)`).
콜백 디커플링은 모범과 일치하나, **논리 액션 계층이 없다** — 키 재매핑이 코드 수정.

### 2.3 본 설계의 선택

- 액션 계층을 둔다 (모범 1).
- press·release(이산) + held(연속) 분리 (모범 2).
- 액션 어휘를 입력 모듈 밖에 둔다 (모범 3) — C++ 타입 안전 수단으로 **템플릿**. 이미 코드베이스가
  `SceneGraph<TId>`로 같은 패턴을 썼다.
- Unreal식 Trigger 페이즈 다양화·Modifier 체인은 학습 프로젝트엔 과함 — 채택 안 함.

## 3. 모듈 구조

`src/input/` → CMake 타겟 `sjhopengl_input`, alias `SJH::input` (STATIC).

| 파일 | 내용 |
|---|---|
| `keyboard_input.h` | `template<typename TAction> class KeyboardInput` — **헤더 온리 템플릿** |
| `mouse_input.h` / `mouse_input.cpp` | `class MouseInput` — 비-템플릿 |
| `CMakeLists.txt` | `add_library(sjhopengl_input STATIC mouse_input.cpp)` |

- 컴파일 단위는 `mouse_input.cpp` 하나 — `KeyboardInput` 은 템플릿이라 헤더 온리
  (`object` 모듈의 `scene_graph.h` 헤더 온리 + `scene_node.cpp` 컴파일 단위와 동형).
- 의존: `glfw` PUBLIC — 두 헤더가 `GLFWwindow*` 와 GLFW 키/버튼 상수를 노출.
- **입력 모듈은 액션 enum 을 정의하지 않는다.** 소비자가 소유.

## 4. 소비자 액션 어휘 — `src/context/game_action.h`

```cpp
#ifndef __SJH_GAME_ACTION_H__
#define __SJH_GAME_ACTION_H__

namespace SJH
{
    /// @brief 게임의 논리 입력 액션 어휘. 입력 모듈이 아닌 *소비자* 가 소유 — 자유롭게 확장.
    enum class GameAction
    {
        MoveForward,
        MoveBackward,
        MoveLeft,
        MoveRight,
        MoveUp,
        MoveDown,
        LookAround,
        Count
    };
}
#endif // __SJH_GAME_ACTION_H__
```

- `Context`(현재 유일 소비자)가 소유. `Attack`/`Jump`/`UIClick` 추가 = 이 파일 한 줄,
  **입력 모듈 불변** — open/closed 보장.
- 향후 별도 gameplay 모듈로 이동·개명 가능 (입력 모듈은 영향 없음).

## 5. `KeyboardInput<TAction>` — `src/input/keyboard_input.h`

```cpp
template <typename TAction>
class KeyboardInput
{
public:
    /// @brief 1단 — 물리 GLFW 키 → 논리 액션 (InputMap 계층).
    void BindKey(int glfwKey, TAction action);
    /// @brief @p glfwKey 바인딩 제거.
    void UnbindKey(int glfwKey);

    /// @brief 2단 — 액션이 *눌려 있는 동안* 매 프레임 실행할 핸들러 (연속).
    void BindHeldHandler(TAction action, std::function<void()> handler);
    /// @brief 2단 — 액션 키가 *눌리는 순간* 1회 실행할 핸들러 (이산, key down).
    void BindPressHandler(TAction action, std::function<void()> handler);
    /// @brief 2단 — 액션 키가 *떼지는 순간* 1회 실행할 핸들러 (이산, key up).
    void BindReleaseHandler(TAction action, std::function<void()> handler);

    /// @brief 매 프레임 — 바인딩된 키 중 현재 눌린 키의 액션을 held 핸들러로 디스패치.
    void PollHeld(GLFWwindow *window);
    /// @brief GLFW key 콜백 위임 — GLFW_PRESS 면 press, GLFW_RELEASE 면 release 핸들러.
    void Dispatch(int glfwKey, int glfwAction);

private:
    std::unordered_map<int, TAction> mKeyBindings;
    std::unordered_map<TAction, std::function<void()>> mHeldHandlers;
    std::unordered_map<TAction, std::function<void()>> mPressHandlers;
    std::unordered_map<TAction, std::function<void()>> mReleaseHandlers;
};
```

- `<unordered_map>` 의 `std::hash` 는 C++14+ 부터 enum 타입을 지원 → `unordered_map<TAction,…>`
  는 `enum class` 에 그대로 동작 (프로젝트는 C++17).
- `PollHeld`: `mKeyBindings` 순회, `glfwGetKey(window, key) == GLFW_PRESS` 인 키의 액션을
  `mHeldHandlers` 에서 찾아 호출.
- `Dispatch`: `mKeyBindings` 로 키→액션 해석 후 — `glfwAction == GLFW_PRESS` 면
  `mPressHandlers`, `GLFW_RELEASE` 면 `mReleaseHandlers` 호출. `GLFW_REPEAT` 은 무시
  (연속은 `PollHeld` 담당).
- 미바인딩 키·미바인딩 액션은 무동작 (맵 miss).

## 6. `MouseInput` — `src/input/mouse_input.{h,cpp}`

```cpp
class MouseInput
{
public:
    /// @brief 드래그 중 이동마다 (dx, dy) 로 호출할 핸들러 (논리적으로 LookAround 액션).
    void BindLookHandler(std::function<void(double dx, double dy)> handler);

    /// @brief GLFW mouse-button 콜백 위임 — 드래그 버튼 press 시 드래그 시작, release 시 종료.
    void HandleButton(int button, int action, double x, double y);
    /// @brief GLFW cursor-pos 콜백 위임 — 드래그 중이면 delta 산출 후 look 핸들러 호출.
    void HandleMove(double x, double y);

    /// @brief 드래그 강제 해제.
    void CancelDrag();
    /// @brief 현재 드래그(=시점 조작) 중인지.
    bool IsDragging() const { return mIsDragging; }

private:
    std::function<void(double, double)> mLookHandler;
    bool   mIsDragging = false;
    double mLastX = 0.0;
    double mLastY = 0.0;
    int    mDragButton = GLFW_MOUSE_BUTTON_RIGHT;   ///< 현 프로젝트 우클릭 유지
};
```

- SSU `MouseInput`(`isDragging`/`lastX`/`lastY`/`dragAction`) 1:1 적응. GLUT→GLFW.
- 마우스는 단일 액션(시점 조작)이라 액션 enum 라우팅 없이 직접 핸들러 — 비-템플릿.
- `HandleButton`: `button == mDragButton && action == GLFW_PRESS` → `mIsDragging=true`,
  `mLastX/Y` 갱신. `GLFW_RELEASE` → `mIsDragging=false`.
- `HandleMove`: `mIsDragging && mLookHandler` 면 `dx=x-mLastX, dy=y-mLastY`, `mLastX/Y` 갱신,
  `mLookHandler(dx, dy)`.
- `mIsDragging` 이 현재 `Camera::IsCamControl` 을 대체.

## 7. Context 리팩토링

- `context.h`: 멤버 `KeyboardInput<GameAction> mKeyboard;` + `MouseInput mMouse;` 추가.
  `mPrevMousePos` 제거 (MouseInput 이 `mLastX/Y` 소유).
- `camera.h`: `bool IsCamControl` 멤버 **제거** — 입력 상태는 `MouseInput::IsDragging()` 이 진실.
- `Context::Init()`: 바인딩 1회 구성 —
  ```cpp
  mKeyboard.BindKey(GLFW_KEY_W, GameAction::MoveForward);
  // … S/A/D/E/Q ↔ MoveBackward/Left/Right/Up/Down
  mKeyboard.BindHeldHandler(GameAction::MoveForward,
                            [this]{ mCamera.Pos += kCameraSpeed * mCamera.GetFront(); });
  // … 나머지 이동 핸들러 (right/up 벡터는 람다 내 산출)
  mMouse.BindLookHandler([this](double dx, double dy) {
      mCamera.EulerYaw   -= dx * kCameraRotSpeed;
      mCamera.EulerPitch -= dy * kCameraRotSpeed;
      // yaw [0,360) 정규화 / pitch [-89,89] 클램프
  });
  ```
- `Context::ProcessInput(window)` → `if (mMouse.IsDragging()) mKeyboard.PollHeld(window);`
  (현재의 "드래그 중에만 이동" 거동 유지).
- `Context::MouseMove(x,y)` → `mMouse.HandleMove(x, y);`
- `Context::MouseButton(b,a,x,y)` → `mMouse.HandleButton(b, a, x, y);` (현 `MouseButton` 의
  버튼별 `spdlog::info` 디버그 로깅은 노이즈 — 제거. `MouseInput` 은 로그를 남기지 않는다.)
- `Context::Reshape` — 입력 아님, 무변경.
- `app/main.cpp` 에 GLFW key 콜백이 있으면 `mKeyboard.Dispatch` 로 연결 (press 경로). 없으면
  press 바인딩은 준비만 — 본 리팩토링은 카메라 이동(held)·시점(드래그)만 실사용.

이동 속도(`kCameraSpeed`)·회전 속도(`kCameraRotSpeed`) 상수는 `context.cpp` 의 익명 namespace
또는 `Const::` 로 둔다 (기존 inline 상수와 동일 위치 정책).

## 8. 테스트

둘 다 GL 컨텍스트 불요 — `glfw` 헤더의 상수(int)만 쓰고 GL 호출 없음.

### `test/test_keyboard_input.cpp`

자체 `enum class TestAction : int { Jump, Crouch, Count }` 로 `KeyboardInput<TestAction>` 검증
(`test_scene_graph.cpp` 가 `TestNode` 를 쓰는 것과 동형):

- `BindKey` + `BindPressHandler` + `Dispatch(key, GLFW_PRESS)` → press 핸들러 1회 실행.
- `BindKey` + `BindReleaseHandler` + `Dispatch(key, GLFW_RELEASE)` → release 핸들러 1회 실행.
- 페이즈 격리: press 만 바인딩 후 `Dispatch(key, GLFW_RELEASE)` → 무동작 / release 만
  바인딩 후 `Dispatch(key, GLFW_PRESS)` → 무동작.
- `Dispatch(key, GLFW_REPEAT)` → press·release 핸들러 모두 무동작.
- 키→액션 해석: 다른 키를 같은 액션에 바인딩 → 둘 다 동작.
- 미바인딩 키 `Dispatch` → 무동작 (크래시 없음).
- `UnbindKey` 후 `Dispatch` → 무동작.
- `BindHeldHandler` 등록 자체는 검증하되, `PollHeld` 는 `glfwGetKey`+윈도우 필요 → 단위 테스트
  제외, 앱 실행으로 검증.

### `test/test_mouse_input.cpp`

- `BindLookHandler` + `HandleButton(RIGHT, PRESS, x, y)` + `HandleMove(x', y')` → 핸들러가
  `(dx, dy) = (x'-x, y'-y)` 로 호출.
- 드래그 시작 없이 `HandleMove` → 핸들러 미호출.
- `HandleButton(RIGHT, RELEASE, …)` 후 `HandleMove` → 핸들러 미호출.
- `CancelDrag` → `IsDragging() == false`, 이후 `HandleMove` 무호출.
- 연속 `HandleMove` — delta 가 *직전 위치* 기준 (누적 아님).
- 드래그 버튼 아닌 버튼(LEFT) press → 드래그 시작 안 함.

### 입력 거동 회귀

리팩토링 전후 입력 거동(WASD/QE 이동, 우클릭 드래그 시점 회전)은 동일해야 한다. 앱 실행으로
육안 확인.

## 9. 영향 파일

| 분류 | 파일 |
|---|---|
| 신규 | `src/input/keyboard_input.h` (헤더 온리 템플릿) |
| 신규 | `src/input/mouse_input.h`, `src/input/mouse_input.cpp` |
| 신규 | `src/input/CMakeLists.txt` |
| 신규 | `src/context/game_action.h` |
| 신규 | `test/test_keyboard_input.cpp`, `test/test_mouse_input.cpp` |
| 수정 | `src/CMakeLists.txt` — `add_subdirectory(input)` |
| 수정 | `app/CMakeLists.txt` — `SJH::input` 링크 (Context 가 헤더에서 노출하므로) |
| 수정 | `src/context/CMakeLists.txt` — `SJH::input` 링크 |
| 수정 | `src/context/context.h`, `src/context/context.cpp` — 입력 위임 |
| 수정 | `src/object/camera.h` — `IsCamControl` 제거 |
| 수정 | `test/CMakeLists.txt` — `test_keyboard_input` / `test_mouse_input` + umbrella |
| 문서 | `.claude/MEMORY.md`, `.claude/architecture.md` — 모듈 인벤토리 (`SJH::input` 추가) |
| 문서 | `doc/pages/00-mainpage.md` — doxygen 클래스 그래프 (`doxygen-class-graph` skill) |

`Context::MouseButton` 시그니처에 `glfwAction` 이 GLFW_PRESS/RELEASE 상수로 들어오므로
`MouseInput::HandleButton` 도 동일 규약. `src/context/CMakeLists.txt` 는 `SJH::input` 을
PUBLIC 으로 링크 — `context.h` 가 `KeyboardInput<GameAction>` 멤버를 노출하기 때문.

## 10. 범위 밖 (Non-Goals)

- Camera/Light 의 Scene Graph 노드 편입 — 후속 사이클 (본 설계가 전제를 마련).
- Unreal식 Trigger 페이즈 다양화 (Tap/Hold/Combo) — 학습 프로젝트엔 과함.
- Modifier 체인 (DeadZone/Negate/Scalar) — 현재 마우스 부호 반전·속도 곱은 람다 안에서 충분.
- 다중 input context / action map 전환 (UI 입력 vs 게임플레이 입력) — 단일 `GameAction` 으로
  충분. 본 설계가 향후 컨텍스트 다중화를 막지 않음.
- 게임패드/터치 — GLFW 키보드·마우스만.
- 키 바인딩의 런타임 설정 파일 로드 — `BindKey` 가 코드 호출. 데이터 로드는 향후.

## 11. 검토했으나 채택하지 않은 대안

| 대안 | 기각 사유 |
|---|---|
| 액션 enum 을 입력 모듈 안에 정의 (`src/input/input_action.h`) | 게임플레이 어휘(`Attack` 등)가 입력 모듈에 누수 — 계층 위반 + open/closed 위반 |
| 문자열 키 액션 (Godot 모델) | 가장 엔진스럽고 런타임 설정까지 열리나 stringly-typed — 오타를 컴파일러가 못 잡음 |
| 불투명 정수 ID (`using ActionId = int`) | 비-템플릿이라 단순하나 경계에서 타입 안전 상실 |
| SSU식 직접 key→`std::function` (액션 계층 없음) | 키 재매핑이 코드 수정 — 엔진 모범의 핵심(논리 액션)을 놓침 |
| `KeyboardInput`/`MouseInput` 를 한 파일 `input.{h,cpp}` 로 (SSU 처럼) | `KeyboardInput` 은 템플릿(헤더), `MouseInput` 은 컴파일 단위 — 분리가 자연스러움 |

## 12. 구현 Phase 분해 (구현 계획서에서 상세화)

빌드가 매 단계 green 을 유지하도록:

1. **`MouseInput` + `src/input/` 모듈 골격** — `mouse_input.{h,cpp}`, `src/input/CMakeLists.txt`
   (`add_library(sjhopengl_input STATIC mouse_input.cpp)`), 루트 `add_subdirectory(input)`,
   `test_mouse_input` 등록. TDD. 소비자 없이 독립 빌드·테스트.
2. **`KeyboardInput<TAction>`** — `keyboard_input.h` 헤더 온리 템플릿 (새 컴파일 단위 없음 —
   모듈 CMake 무변경), `test_keyboard_input` (자체 `TestAction` enum). TDD.
3. **`GameAction` + Context 리팩토링** — `src/context/game_action.h`, `Context` 멤버 추가,
   `Init` 바인딩, `ProcessInput`/`MouseMove`/`MouseButton` 위임, `Camera::IsCamControl` 제거,
   `src/context/CMakeLists.txt`·`app/CMakeLists.txt` 에 `SJH::input` 링크. 앱 실행 회귀 확인.
4. **문서 갱신** — MEMORY / architecture / doxygen 클래스 그래프.
