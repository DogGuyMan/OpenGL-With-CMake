# 입력 모듈 (`SJH::input`) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `Context`의 GLFW 입력 처리를 독립 모듈 `SJH::input`(논리 액션 계층 + 콜백 바인딩)으로 추출한다.

**Architecture:** `MouseInput`(비-템플릿, 드래그→delta 콜백) + `KeyboardInput<TAction>`(헤더 온리 템플릿, 키→논리 액션→핸들러 2단). 액션 enum은 입력 모듈이 아닌 소비자(`Context`의 `GameAction`)가 소유 — 입력 모듈은 액션 어휘를 모른다. `Context`는 입력 메서드를 두 클래스에 위임하고 `Init`에서 카메라 액션을 바인딩한다.

**Tech Stack:** C++17, CMake + Make, vcpkg, Catch2 v3, GLFW

**설계 출처:** [doc/design/2026-05-19-input-module-design.md](2026-05-19-input-module-design.md)

**공통 빌드/테스트 명령:**
```bash
# CMakeLists.txt 변경 후 — 재구성 (vcpkg 스킵)
cmake --preset debug -DVCPKG_MANIFEST_INSTALL=OFF
# 코드만 수정
cmake --build build_Darwin
# 특정 테스트 빌드 (test 는 EXCLUDE_FROM_ALL)
cmake --build build_Darwin --target test_mouse_input
# 테스트 실행 — 실행파일은 build_Darwin/test/ 에 생성
./build_Darwin/test/test_mouse_input
```

---

## Task 1: `MouseInput` + `src/input/` 모듈 골격

`MouseInput` 클래스와 `src/input/` 모듈을 신설한다. 소비자가 없으므로 독립적으로 빌드·테스트된다.

**Files:**
- Create: `src/input/mouse_input.h`
- Create: `src/input/mouse_input.cpp`
- Create: `src/input/CMakeLists.txt`
- Modify: `src/CMakeLists.txt`
- Create: `test/test_mouse_input.cpp`
- Modify: `test/CMakeLists.txt`

- [ ] **Step 1: `src/input/mouse_input.h` 작성**

```cpp
/**
 * @file mouse_input.h
 * @brief 마우스 드래그 → (dx, dy) delta 콜백. SSU MouseInput 의 GLFW 적응.
 */
#ifndef __SJH_MOUSE_INPUT_H__
#define __SJH_MOUSE_INPUT_H__

#include <GLFW/glfw3.h>
#include <functional>

namespace SJH
{
    /**
     * @brief 마우스 드래그 입력 — 드래그 버튼(기본 우클릭) press~release 동안
     *        cursor 이동마다 look 핸들러를 (dx, dy) 로 호출.
     * @details `mIsDragging` 이 "시점 조작 중" 상태의 단일 진실 — 소비자는 별도 플래그 불요.
     */
    class MouseInput
    {
    public:
        /// @brief 드래그 중 이동마다 (dx, dy) 로 호출할 핸들러 (논리적으로 LookAround 액션).
        void BindLookHandler(std::function<void(double dx, double dy)> handler);

        /// @brief GLFW mouse-button 콜백 위임 — 드래그 버튼 press 시 시작, release 시 종료.
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
        int    mDragButton = GLFW_MOUSE_BUTTON_RIGHT;
    };
}
#endif // __SJH_MOUSE_INPUT_H__
```

- [ ] **Step 2: `src/input/mouse_input.cpp` 작성**

```cpp
#include "input/mouse_input.h"
#include <utility>

namespace SJH
{
    void MouseInput::BindLookHandler(std::function<void(double, double)> handler)
    {
        mLookHandler = std::move(handler);
    }

    void MouseInput::HandleButton(int button, int action, double x, double y)
    {
        if (button != mDragButton)
            return;
        if (action == GLFW_PRESS)
        {
            mIsDragging = true;
            mLastX = x;
            mLastY = y;
        }
        else if (action == GLFW_RELEASE)
        {
            mIsDragging = false;
        }
    }

    void MouseInput::HandleMove(double x, double y)
    {
        if (!mIsDragging)
            return;
        const double dx = x - mLastX;
        const double dy = y - mLastY;
        mLastX = x;
        mLastY = y;
        if (mLookHandler)
            mLookHandler(dx, dy);
    }

    void MouseInput::CancelDrag()
    {
        mIsDragging = false;
    }
}
```

- [ ] **Step 3: `src/input/CMakeLists.txt` 작성**

```cmake
add_library(sjhopengl_input STATIC mouse_input.cpp)
add_library(SJH::input ALIAS sjhopengl_input)

target_include_directories(sjhopengl_input
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sjhopengl_input
    PUBLIC  glfw      # mouse_input.h / keyboard_input.h: <GLFW/glfw3.h>, 키·버튼 상수, GLFWwindow*
)

target_compile_features(sjhopengl_input PUBLIC cxx_std_17)
```

> `keyboard_input.h`(Task 2)는 헤더 온리 템플릿이라 새 소스가 없다 — 본 CMakeLists 는 Task 2 에서
> 변경되지 않는다.

- [ ] **Step 4: `src/CMakeLists.txt` 에 모듈 추가**

`src/CMakeLists.txt` 의 `add_subdirectory(resource_registry)` 줄 다음에 추가:

```cmake
add_subdirectory(input)
```

(CMake 는 `target_link_libraries` 의 타겟명을 generate 시점에 해석하므로 `add_subdirectory` 순서는
무관 — 기존 `context` 가 9번째 줄에서 뒤쪽 모듈을 링크하는 것과 동일.)

- [ ] **Step 5: `test/test_mouse_input.cpp` 작성**

```cpp
/**
 * @file test_mouse_input.cpp
 * @brief SJH::MouseInput — 드래그 → delta 콜백 (GL 컨텍스트 불요).
 */
#include "input/mouse_input.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("MouseInput 드래그 — press 후 move 가 delta 콜백", "[mouse_input]")
{
    SJH::MouseInput mouse;
    double gotDx = 0.0, gotDy = 0.0;
    int calls = 0;
    mouse.BindLookHandler([&](double dx, double dy) { gotDx = dx; gotDy = dy; ++calls; });

    mouse.HandleButton(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS, 100.0, 200.0);
    REQUIRE(mouse.IsDragging());
    mouse.HandleMove(110.0, 195.0);

    REQUIRE(calls == 1);
    REQUIRE_THAT(gotDx, WithinAbs(10.0, 1e-9));
    REQUIRE_THAT(gotDy, WithinAbs(-5.0, 1e-9));
}

TEST_CASE("MouseInput 비드래그 — press 없이 move 는 무호출", "[mouse_input]")
{
    SJH::MouseInput mouse;
    int calls = 0;
    mouse.BindLookHandler([&](double, double) { ++calls; });
    mouse.HandleMove(50.0, 50.0);
    REQUIRE(calls == 0);
    REQUIRE_FALSE(mouse.IsDragging());
}

TEST_CASE("MouseInput release — 드래그 종료 후 move 무호출", "[mouse_input]")
{
    SJH::MouseInput mouse;
    int calls = 0;
    mouse.BindLookHandler([&](double, double) { ++calls; });
    mouse.HandleButton(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS, 0.0, 0.0);
    mouse.HandleButton(GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE, 5.0, 5.0);
    REQUIRE_FALSE(mouse.IsDragging());
    mouse.HandleMove(20.0, 20.0);
    REQUIRE(calls == 0);
}

TEST_CASE("MouseInput CancelDrag — 강제 해제", "[mouse_input]")
{
    SJH::MouseInput mouse;
    int calls = 0;
    mouse.BindLookHandler([&](double, double) { ++calls; });
    mouse.HandleButton(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS, 0.0, 0.0);
    mouse.CancelDrag();
    REQUIRE_FALSE(mouse.IsDragging());
    mouse.HandleMove(30.0, 30.0);
    REQUIRE(calls == 0);
}

TEST_CASE("MouseInput 연속 move — delta 는 직전 위치 기준 (누적 아님)", "[mouse_input]")
{
    SJH::MouseInput mouse;
    double lastDx = 0.0;
    mouse.BindLookHandler([&](double dx, double) { lastDx = dx; });
    mouse.HandleButton(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS, 0.0, 0.0);
    mouse.HandleMove(10.0, 0.0);
    REQUIRE_THAT(lastDx, WithinAbs(10.0, 1e-9));
    mouse.HandleMove(13.0, 0.0);
    REQUIRE_THAT(lastDx, WithinAbs(3.0, 1e-9)); // 13-10 — 누적이면 13
}

TEST_CASE("MouseInput 드래그 버튼 아닌 버튼 — 드래그 시작 안 함", "[mouse_input]")
{
    SJH::MouseInput mouse;
    mouse.HandleButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0.0, 0.0);
    REQUIRE_FALSE(mouse.IsDragging());
}
```

- [ ] **Step 6: `test/CMakeLists.txt` 에 테스트 등록**

`test/CMakeLists.txt` 의 `test_light` 블록(`catch_discover_tests(test_light)` 줄) 다음, 또는
`test_transform` 블록 근처 일관된 위치에 추가:

```cmake
#  input 모듈 — MouseInput 드래그 (GL 불요)
add_executable(test_mouse_input test_mouse_input.cpp)
target_link_libraries(test_mouse_input PRIVATE
    Catch2::Catch2WithMain
    SJH::input
)
target_compile_features(test_mouse_input PRIVATE cxx_std_17)
catch_discover_tests(test_mouse_input)
```

`add_custom_target(tests DEPENDS ...)` 목록에 `test_mouse_input` 추가.

- [ ] **Step 7: 재구성 + 빌드 + 테스트**

Run:
```bash
cmake --preset debug -DVCPKG_MANIFEST_INSTALL=OFF
cmake --build build_Darwin --target test_mouse_input
./build_Darwin/test/test_mouse_input
cmake --build build_Darwin
```
Expected: `test_mouse_input` 6개 케이스 전부 PASS. `cmake --build build_Darwin`(app+src) 성공
— `sjhopengl_input` 이 ALL 에 포함되어 함께 빌드되며, 아직 아무도 링크하지 않으므로 회귀 없음.

- [ ] **Step 8: 커밋**

```bash
git add src/input/mouse_input.h src/input/mouse_input.cpp src/input/CMakeLists.txt \
        src/CMakeLists.txt test/test_mouse_input.cpp test/CMakeLists.txt
git commit -m "$(printf '%s\n' \
  '[feat] : SJH::input 모듈 신설 — MouseInput (드래그 → delta 콜백)' '' \
  'SSU MouseInput 의 GLFW 적응. isDragging 이 시점 조작 상태의 단일 진실.' \
  'test_mouse_input 6 케이스.' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Task 2: `KeyboardInput<TAction>` — 헤더 온리 템플릿

키→논리 액션→핸들러 2단 디스패처. 액션 타입에 제네릭 — 입력 모듈은 액션 어휘를 모른다.
템플릿이므로 헤더 온리 — 새 컴파일 단위 없음, 모듈 CMake 무변경.

**Files:**
- Create: `src/input/keyboard_input.h`
- Create: `test/test_keyboard_input.cpp`
- Modify: `test/CMakeLists.txt`

- [ ] **Step 1: `src/input/keyboard_input.h` 작성**

```cpp
/**
 * @file keyboard_input.h
 * @brief 키 → 논리 액션 → 핸들러 2단 디스패처 (액션 타입에 제네릭).
 */
#ifndef __SJH_KEYBOARD_INPUT_H__
#define __SJH_KEYBOARD_INPUT_H__

#include <GLFW/glfw3.h>
#include <functional>
#include <unordered_map>
#include <utility>

namespace SJH
{
    /**
     * @brief 키보드 입력 — 물리 키를 논리 액션으로 매핑하고 액션에 핸들러를 바인딩.
     * @tparam TAction 소비자가 정의하는 액션 enum. 입력 모듈은 이 어휘를 알지 못한다.
     * @details
     *  - 1단: @ref BindKey — 물리 GLFW 키 ↔ 논리 액션 (InputMap 계층).
     *  - 2단: @ref BindHeldHandler / @ref BindPressHandler / @ref BindReleaseHandler — 액션 ↔ 핸들러.
     *  - 연속(held)은 @ref PollHeld, 이산(press/release)은 @ref Dispatch.
     */
    template <typename TAction>
    class KeyboardInput
    {
    public:
        /// @brief 물리 키 → 논리 액션 바인딩 (같은 키 재바인딩 시 덮어씀).
        void BindKey(int glfwKey, TAction action) { mKeyBindings[glfwKey] = action; }
        /// @brief @p glfwKey 의 키→액션 바인딩 제거.
        void UnbindKey(int glfwKey) { mKeyBindings.erase(glfwKey); }

        /// @brief 액션이 *눌려 있는 동안* 매 프레임 실행할 핸들러 (연속).
        void BindHeldHandler(TAction action, std::function<void()> handler)
        {
            mHeldHandlers[action] = std::move(handler);
        }
        /// @brief 액션 키가 *눌리는 순간* 1회 실행할 핸들러 (이산, key down).
        void BindPressHandler(TAction action, std::function<void()> handler)
        {
            mPressHandlers[action] = std::move(handler);
        }
        /// @brief 액션 키가 *떼지는 순간* 1회 실행할 핸들러 (이산, key up).
        void BindReleaseHandler(TAction action, std::function<void()> handler)
        {
            mReleaseHandlers[action] = std::move(handler);
        }

        /// @brief 매 프레임 — 바인딩된 키 중 현재 눌린 키의 액션을 held 핸들러로 디스패치.
        void PollHeld(GLFWwindow *window)
        {
            for (const auto &[glfwKey, action] : mKeyBindings)
            {
                if (glfwGetKey(window, glfwKey) != GLFW_PRESS)
                    continue;
                auto it = mHeldHandlers.find(action);
                if (it != mHeldHandlers.end() && it->second)
                    it->second();
            }
        }

        /// @brief GLFW key 콜백 위임 — GLFW_PRESS 면 press, GLFW_RELEASE 면 release 핸들러.
        ///        GLFW_REPEAT 등 그 외 action 은 무시 (연속은 @ref PollHeld 담당).
        void Dispatch(int glfwKey, int glfwAction)
        {
            auto keyIt = mKeyBindings.find(glfwKey);
            if (keyIt == mKeyBindings.end())
                return;
            const TAction action = keyIt->second;

            std::unordered_map<TAction, std::function<void()>> *table = nullptr;
            if (glfwAction == GLFW_PRESS)
                table = &mPressHandlers;
            else if (glfwAction == GLFW_RELEASE)
                table = &mReleaseHandlers;
            else
                return;

            auto it = table->find(action);
            if (it != table->end() && it->second)
                it->second();
        }

    private:
        std::unordered_map<int, TAction> mKeyBindings;
        std::unordered_map<TAction, std::function<void()>> mHeldHandlers;
        std::unordered_map<TAction, std::function<void()>> mPressHandlers;
        std::unordered_map<TAction, std::function<void()>> mReleaseHandlers;
    };
}
#endif // __SJH_KEYBOARD_INPUT_H__
```

> `std::hash` 는 C++14+ 부터 enum 타입을 지원하므로 `unordered_map<TAction, …>` 은
> `enum class` 에 그대로 동작 (프로젝트는 C++17).

- [ ] **Step 2: `test/test_keyboard_input.cpp` 작성**

```cpp
/**
 * @file test_keyboard_input.cpp
 * @brief SJH::KeyboardInput<TAction> — 키→액션→핸들러 디스패치 (GL 컨텍스트 불요).
 */
#include "input/keyboard_input.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
    // KeyboardInput 은 TAction 이 enum 이라는 점만 가정 — 테스트용 액션 enum.
    enum class TestAction { Jump, Crouch };
}

TEST_CASE("KeyboardInput press — BindKey+BindPressHandler+Dispatch(PRESS)", "[keyboard_input]")
{
    SJH::KeyboardInput<TestAction> kb;
    int jumps = 0;
    kb.BindKey(GLFW_KEY_SPACE, TestAction::Jump);
    kb.BindPressHandler(TestAction::Jump, [&] { ++jumps; });

    kb.Dispatch(GLFW_KEY_SPACE, GLFW_PRESS);
    REQUIRE(jumps == 1);
}

TEST_CASE("KeyboardInput release — Dispatch(RELEASE) 가 release 핸들러", "[keyboard_input]")
{
    SJH::KeyboardInput<TestAction> kb;
    int releases = 0;
    kb.BindKey(GLFW_KEY_SPACE, TestAction::Jump);
    kb.BindReleaseHandler(TestAction::Jump, [&] { ++releases; });

    kb.Dispatch(GLFW_KEY_SPACE, GLFW_RELEASE);
    REQUIRE(releases == 1);
}

TEST_CASE("KeyboardInput 페이즈 격리 — press 만 바인딩 시 RELEASE/REPEAT 무동작", "[keyboard_input]")
{
    SJH::KeyboardInput<TestAction> kb;
    int presses = 0;
    kb.BindKey(GLFW_KEY_SPACE, TestAction::Jump);
    kb.BindPressHandler(TestAction::Jump, [&] { ++presses; });

    kb.Dispatch(GLFW_KEY_SPACE, GLFW_RELEASE); // release 핸들러 없음
    REQUIRE(presses == 0);
    kb.Dispatch(GLFW_KEY_SPACE, GLFW_REPEAT);  // repeat 은 무시
    REQUIRE(presses == 0);
    kb.Dispatch(GLFW_KEY_SPACE, GLFW_PRESS);
    REQUIRE(presses == 1);
}

TEST_CASE("KeyboardInput 키→액션 해석 — 두 키를 같은 액션에", "[keyboard_input]")
{
    SJH::KeyboardInput<TestAction> kb;
    int jumps = 0;
    kb.BindKey(GLFW_KEY_SPACE, TestAction::Jump);
    kb.BindKey(GLFW_KEY_W, TestAction::Jump);
    kb.BindPressHandler(TestAction::Jump, [&] { ++jumps; });

    kb.Dispatch(GLFW_KEY_SPACE, GLFW_PRESS);
    kb.Dispatch(GLFW_KEY_W, GLFW_PRESS);
    REQUIRE(jumps == 2);
}

TEST_CASE("KeyboardInput 미바인딩 키 — Dispatch 무동작 (크래시 없음)", "[keyboard_input]")
{
    SJH::KeyboardInput<TestAction> kb;
    kb.Dispatch(GLFW_KEY_ESCAPE, GLFW_PRESS); // 바인딩 없음
    SUCCEED("미바인딩 키 Dispatch 가 안전");
}

TEST_CASE("KeyboardInput UnbindKey — 해제 후 Dispatch 무동작", "[keyboard_input]")
{
    SJH::KeyboardInput<TestAction> kb;
    int jumps = 0;
    kb.BindKey(GLFW_KEY_SPACE, TestAction::Jump);
    kb.BindPressHandler(TestAction::Jump, [&] { ++jumps; });
    kb.UnbindKey(GLFW_KEY_SPACE);

    kb.Dispatch(GLFW_KEY_SPACE, GLFW_PRESS);
    REQUIRE(jumps == 0);
}
```

- [ ] **Step 3: `test/CMakeLists.txt` 에 테스트 등록**

`test_mouse_input` 블록 다음에 추가:

```cmake
#  input 모듈 — KeyboardInput<TAction> 디스패치 (GL 불요)
add_executable(test_keyboard_input test_keyboard_input.cpp)
target_link_libraries(test_keyboard_input PRIVATE
    Catch2::Catch2WithMain
    SJH::input
)
target_compile_features(test_keyboard_input PRIVATE cxx_std_17)
catch_discover_tests(test_keyboard_input)
```

`add_custom_target(tests DEPENDS ...)` 목록에 `test_keyboard_input` 추가.

- [ ] **Step 4: 재구성 + 빌드 + 테스트**

Run:
```bash
cmake --preset debug -DVCPKG_MANIFEST_INSTALL=OFF
cmake --build build_Darwin --target test_keyboard_input
./build_Darwin/test/test_keyboard_input
```
Expected: `test_keyboard_input` 6개 케이스 전부 PASS.

- [ ] **Step 5: 커밋**

```bash
git add src/input/keyboard_input.h test/test_keyboard_input.cpp test/CMakeLists.txt
git commit -m "$(printf '%s\n' \
  '[feat] : KeyboardInput<TAction> — 키→논리 액션→핸들러 2단 디스패처' '' \
  '액션 타입에 제네릭 — 입력 모듈이 게임 어휘 비소유. press/release/held 3 페이즈.' \
  '헤더 온리 템플릿. test_keyboard_input 6 케이스.' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Task 3: `GameAction` + Context 리팩토링

소비자 액션 어휘 `GameAction` 을 정의하고, `Context` 의 입력 처리를 입력 모듈에 위임한다.
입력 거동(WASD/QE 이동, 우클릭 드래그 시점 회전)은 리팩토링 전후 동일해야 한다.

**Files:**
- Create: `src/context/game_action.h`
- Modify: `src/object/camera.h`
- Modify: `src/context/context.h`
- Modify: `src/context/context.cpp`
- Modify: `src/context/CMakeLists.txt`
- Modify: `app/CMakeLists.txt`

- [ ] **Step 1: `src/context/game_action.h` 작성**

```cpp
/**
 * @file game_action.h
 * @brief 게임의 논리 입력 액션 어휘. 입력 모듈이 아닌 *소비자* 가 소유 — 자유롭게 확장.
 */
#ifndef __SJH_GAME_ACTION_H__
#define __SJH_GAME_ACTION_H__

namespace SJH
{
    /// @brief 논리 입력 액션. KeyboardInput<GameAction> 의 액션 타입. 새 액션은 여기 추가.
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

- [ ] **Step 2: `src/object/camera.h` 에서 `IsCamControl` 제거**

`camera.h` 의 다음 3줄을 삭제한다 (입력 상태는 `MouseInput::IsDragging()` 이 진실):

```cpp
        // === 입력 / 회전 ===
        /// @brief 입력으로 카메라를 조작 중인지 여부. 마우스 좌클릭 PRESS/RELEASE 가 토글.
        bool  IsCamControl{false};
```

→ 다음으로 교체 (섹션 주석만 남김):

```cpp
        // === 회전 (Yaw / Pitch) ===
```

(바로 아래 `EulerPitch` / `EulerYaw` 멤버는 그대로 둔다.)

- [ ] **Step 3: `src/context/context.h` 수정 — include + 멤버**

`#include "common/common.h"` 줄 다음에 추가:

```cpp
#include "context/game_action.h"
#include "input/keyboard_input.h"
#include "input/mouse_input.h"
```

그리고 멤버 선언부에서 `mPrevMousePos` 블록을 입력 모듈 멤버로 교체한다. 다음 2줄 +
주석을 삭제:

```cpp
        /// @brief 직전 프레임 마우스 위치 — 회전 델타 계산용. @ref MouseButton(LEFT, PRESS) 에서 초기화.
        glm::vec2 mPrevMousePos{glm::vec2(0.0f)};
```

→ 다음으로 교체:

```cpp
        /// @brief 키보드 입력 — 물리 키↔GameAction 매핑 + 액션↔핸들러. Init 에서 바인딩.
        KeyboardInput<GameAction> mKeyboard;

        /// @brief 마우스 입력 — 우클릭 드래그 → 시점 회전. IsDragging() 이 조작 상태.
        MouseInput mMouse;
```

(`Camera mCamera;` 멤버는 그대로 둔다.)

- [ ] **Step 4: `src/context/context.cpp` — 입력 상수 추가**

`context.cpp` 의 `namespace SJH` 안, `std::vector<glm::vec3> cubePositions` 정의 *위에*
추가:

```cpp
    namespace
    {
        constexpr float kCameraSpeed    = 0.05f; // WASD/QE 이동 속도 (프레임당 world unit)
        constexpr float kCameraRotSpeed = 0.1f;  // 마우스 드래그 → yaw/pitch 회전 배율
    }
```

- [ ] **Step 5: `context.cpp` — `ProcessInput` / `MouseMove` / `MouseButton` 위임으로 교체**

세 함수의 본문 전체를 다음으로 교체한다:

```cpp
    void Context::ProcessInput(GLFWwindow *window)
    {
        // 우클릭 드래그 중일 때만 카메라 이동 — 기존 IsCamControl 가드와 동일 거동.
        if (mMouse.IsDragging())
            mKeyboard.PollHeld(window);
    }
```

```cpp
    void Context::MouseMove(double x, double y)
    {
        mMouse.HandleMove(x, y);
    }
```

```cpp
    void Context::MouseButton(int button, int action, double x, double y)
    {
        mMouse.HandleButton(button, action, x, y);
    }
```

> `Reshape` 는 입력이 아니므로 무변경. 기존 `MouseButton` 의 버튼별 `spdlog::info` 디버그
> 로깅은 제거된다 (위 교체로). `context.cpp` 의 다른 `spdlog` 사용처(`Init` 의 program id
> 로그)가 있으므로 `#include <spdlog/spdlog.h>` 는 유지.

- [ ] **Step 6: `context.cpp` `Init()` — 입력 바인딩 추가**

`Init()` 의 `mCamera = { … };` 대입 블록 다음(빈 줄 뒤), `mProgram = Program::CreateWithVSFS(…)`
줄 *앞에* 다음 블록을 삽입한다:

```cpp
        // === Input 바인딩 ===
        // 1단 — 물리 키 → 논리 액션 (InputMap 계층).
        mKeyboard.BindKey(GLFW_KEY_W, GameAction::MoveForward);
        mKeyboard.BindKey(GLFW_KEY_S, GameAction::MoveBackward);
        mKeyboard.BindKey(GLFW_KEY_D, GameAction::MoveRight);
        mKeyboard.BindKey(GLFW_KEY_A, GameAction::MoveLeft);
        mKeyboard.BindKey(GLFW_KEY_E, GameAction::MoveUp);
        mKeyboard.BindKey(GLFW_KEY_Q, GameAction::MoveDown);

        // 2단 — 논리 액션 → 핸들러 (연속 이동). 카메라 right/up 은 front 에서 매번 산출.
        mKeyboard.BindHeldHandler(GameAction::MoveForward,
                                  [this] { mCamera.Pos += kCameraSpeed * mCamera.GetFront(); });
        mKeyboard.BindHeldHandler(GameAction::MoveBackward,
                                  [this] { mCamera.Pos -= kCameraSpeed * mCamera.GetFront(); });
        mKeyboard.BindHeldHandler(GameAction::MoveRight, [this] {
            const auto right = glm::normalize(glm::cross(mCamera.CamUp, -mCamera.GetFront()));
            mCamera.Pos += kCameraSpeed * right;
        });
        mKeyboard.BindHeldHandler(GameAction::MoveLeft, [this] {
            const auto right = glm::normalize(glm::cross(mCamera.CamUp, -mCamera.GetFront()));
            mCamera.Pos -= kCameraSpeed * right;
        });
        mKeyboard.BindHeldHandler(GameAction::MoveUp, [this] {
            const auto front = mCamera.GetFront();
            const auto right = glm::normalize(glm::cross(mCamera.CamUp, -front));
            const auto up = glm::normalize(glm::cross(-front, right));
            mCamera.Pos += kCameraSpeed * up;
        });
        mKeyboard.BindHeldHandler(GameAction::MoveDown, [this] {
            const auto front = mCamera.GetFront();
            const auto right = glm::normalize(glm::cross(mCamera.CamUp, -front));
            const auto up = glm::normalize(glm::cross(-front, right));
            mCamera.Pos -= kCameraSpeed * up;
        });

        // 마우스 우클릭 드래그 → 시점 회전 (yaw/pitch). 기존 MouseMove 로직 이식.
        mMouse.BindLookHandler([this](double dx, double dy) {
            mCamera.EulerYaw -= static_cast<float>(dx) * kCameraRotSpeed;
            mCamera.EulerPitch -= static_cast<float>(dy) * kCameraRotSpeed;
            if (mCamera.EulerYaw < 0.0f)
                mCamera.EulerYaw += 360.0f;
            if (mCamera.EulerYaw > 360.0f)
                mCamera.EulerYaw -= 360.0f;
            if (mCamera.EulerPitch > 89.0f)
                mCamera.EulerPitch = 89.0f;
            if (mCamera.EulerPitch < -89.0f)
                mCamera.EulerPitch = -89.0f;
        });
```

> `kCameraSpeed` / `kCameraRotSpeed` 는 Step 4 의 익명 namespace `constexpr` — 컴파일 타임
> 상수라 람다가 캡처 없이 사용 가능하고 `Init()` 종료 후에도 유효하다.

- [ ] **Step 7: `src/context/CMakeLists.txt` — `SJH::input` 링크 추가**

`target_link_libraries(sjhopengl_context PUBLIC …)` 의 PUBLIC 목록에 `SJH::input` 을 추가
(`context.h` 가 `KeyboardInput<GameAction>` / `MouseInput` 멤버를 노출하므로 PUBLIC):

```cmake
target_link_libraries(sjhopengl_context
    PUBLIC  SJH::common
            SJH::shader
            SJH::buffer
            SJH::layout
            SJH::object
            SJH::material
            SJH::resource_registry
            SJH::program
            SJH::input
            glad::glad
            glm::glm
            glfw
            imgui::imgui
    PRIVATE SJH::diagnostics
)
```

- [ ] **Step 8: `app/CMakeLists.txt` — `SJH::input` 링크 추가**

`target_link_libraries(${PROJECT_NAME} PRIVATE …)` 의 내부 모듈 목록에 `SJH::input` 을 추가
(`SJH::context` 다음 줄):

```cmake
    SJH::context
    SJH::input
```

(`SJH::context` 가 `SJH::input` 을 PUBLIC 으로 끌어오므로 transitive 로도 충분하나, 프로젝트가
모든 내부 모듈을 명시 나열하는 컨벤션을 따른다.)

- [ ] **Step 9: 재구성 + 빌드**

Run:
```bash
cmake --preset debug -DVCPKG_MANIFEST_INSTALL=OFF
cmake --build build_Darwin
```
Expected: 빌드 성공. `Camera::IsCamControl` 참조가 남아 있으면 컴파일 에러 — 그 경우 해당
참조를 찾아 제거 (현재 `context.cpp` 의 `ProcessInput`/`MouseMove`/`MouseButton` 외에는
`IsCamControl` 사용처가 없어야 하며, Step 5 가 그 셋을 모두 교체함).

- [ ] **Step 10: 회귀 — 빌드 + 실행 확인**

Run:
```bash
cmake --build build_Darwin --target tests
ctest --test-dir build_Darwin --output-on-failure
./build_Darwin/OpenGL-With-CMake
```
Expected: 전체 테스트 통과. 앱 실행 시 — 우클릭 드래그로 시점 회전, 드래그 중 WASD 이동 /
QE 상하 이동이 리팩토링 전과 동일하게 동작. 확인 후 창을 닫는다.

- [ ] **Step 11: 커밋**

```bash
git add src/context/game_action.h src/object/camera.h src/context/context.h \
        src/context/context.cpp src/context/CMakeLists.txt app/CMakeLists.txt
git commit -m "$(printf '%s\n' \
  '[refactor] : Context 입력 처리를 SJH::input 모듈로 위임' '' \
  'GameAction 어휘 정의, KeyboardInput<GameAction>/MouseInput 멤버 도입.' \
  'ProcessInput/MouseMove/MouseButton 은 얇은 위임. Camera::IsCamControl 제거.' \
  '입력 거동 불변.' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Task 4: 문서 갱신

**Files:**
- Modify: `.claude/MEMORY.md`
- Modify: `.claude/architecture.md`
- Modify: `doc/pages/00-mainpage.md` (`doxygen-class-graph` skill 경유)

> 주의: `.claude/` 는 `.git/info/exclude` 로 git 비추적 — `.claude/*` 갱신은 작업 트리에만
> 반영되고 커밋 대상이 아니다. `doc/pages/00-mainpage.md` 만 커밋된다.

- [ ] **Step 1: `.claude/MEMORY.md` 모듈 인벤토리 갱신**

`### 내부 모듈` 목록에 `SJH::input` 항목을 추가: "`src/input/` → `SJH::input` — STATIC.
`mouse_input.cpp` + `keyboard_input.h`(헤더 온리 템플릿). `KeyboardInput<TAction>`(키→논리
액션→핸들러 2단, 액션 타입 제네릭) + `MouseInput`(드래그→delta). PUBLIC: glfw."

- [ ] **Step 2: `.claude/architecture.md` 모듈 인벤토리 갱신**

§5 모듈 인벤토리 표에 `SJH::input` 행을 추가 — 클래스 `KeyboardInput<TAction>` / `MouseInput`,
역할(입력 디스패치 — 논리 액션 계층, SSU inputs 패턴 + 엔진 모범), 의존 `glfw` (PUBLIC).
`SJH::context` 행의 의존성 목록에 `input` 추가. 설계 출처로
[doc/design/2026-05-19-input-module-design.md](../doc/design/2026-05-19-input-module-design.md)
링크.

- [ ] **Step 3: doxygen 클래스 그래프 갱신**

`doxygen-class-graph` skill 을 호출해 `doc/pages/00-mainpage.md` 의 클래스 의존 그래프에
`KeyboardInput` / `MouseInput` 을 추가한다. `Context` → `KeyboardInput`·`MouseInput` 소유
(실선, value 멤버). 새 cluster `Input` 또는 기존 `Scene` cluster 에 배치.

- [ ] **Step 4: 커밋**

```bash
git add doc/pages/00-mainpage.md
git commit -m "$(printf '%s\n' \
  '[doc] : 입력 모듈 — doxygen 클래스 그래프에 KeyboardInput/MouseInput 추가' '' \
  'Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>')"
```

---

## Self-Review 결과

- **Spec 커버리지:** 설계 §3(모듈 구조)=Task1 Step3-4, §4(GameAction)=Task3 Step1,
  §5(KeyboardInput)=Task2, §6(MouseInput)=Task1, §7(Context 리팩토링)=Task3, §8(테스트)=
  Task1 Step5·Task2 Step2, §9(영향 파일)=전 Task, §12(Phase 분해)=Task1-4 와 1:1. 누락 없음.
- **Placeholder:** 없음 — 모든 코드 블록은 실제 코드, CMake 수정은 정확한 삽입 위치 명시.
- **타입 일관성:** `MouseInput::{BindLookHandler,HandleButton,HandleMove,CancelDrag,IsDragging}`,
  `KeyboardInput<TAction>::{BindKey,UnbindKey,BindHeldHandler,BindPressHandler,BindReleaseHandler,
  PollHeld,Dispatch}`, `GameAction`, `KeyboardInput<GameAction>` — Task 간 시그니처·이름 일치
  확인 완료. `Dispatch(int,int)` 의 GLFW_PRESS/RELEASE/REPEAT 분기와 테스트 케이스 일치.
- **빌드 green 불변식:** Task1 종료 시 `sjhopengl_input` 이 ALL 에 포함돼 빌드되나 미사용 —
  회귀 없음. Task2 는 헤더만 추가 — app/src 무영향. Task3 가 Context 를 입력 모듈에 연결 —
  이 시점에 `cmake --build build_Darwin` + 앱 실행 회귀 확인.
