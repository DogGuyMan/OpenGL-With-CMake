# Framebuffer 입문 — Depth Test 부터

Framebuffer 를 본격적으로 다루기 전, 그 *구성 요소* 중 하나인 **depth buffer** 와 **depth test** 부터 정리한다. 깊이 테스트는 framebuffer 가 단순한 "색 픽셀 배열" 이 아니라 *여러 buffer 의 묶음* 임을 이해하는 가장 자연스러운 입구다.

---

## 1. Framebuffer 란 — 여러 buffer 의 묶음

화면에 보이는 한 장의 그림은 사실 *여러 개의 buffer* 가 겹쳐 만들어진다.

| Buffer | 저장하는 것 | 비트 |
|--------|------------|------|
| **Color buffer** | 픽셀의 RGBA 색 | 보통 32-bit (RGBA8) |
| **Depth buffer** | 픽셀의 *깊이값* (카메라로부터의 거리) | 보통 24-bit |
| **Stencil buffer** | 픽셀별 마스크/태그 | 보통 8-bit (depth 와 같이 32-bit 패킹) |

**Default framebuffer** = 윈도우 화면. GLFW 가 윈도우 생성 시 자동 제공.
**FBO (Framebuffer Object)** = 화면 밖(off-screen) 렌더 타겟. 그림자 맵, 포스트 프로세싱 등에 쓰임 — *§7 의 다음 단계*.

`glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)` 가 *두 비트* 를 함께 지우는 이유 — color 와 depth 는 *별개 buffer* 이기 때문. 매 프레임 둘 다 초기화해야 한다.

---

## 2. Depth Test — 왜 필요한가

depth test 없이 3D 를 그리면, *그리는 순서대로* 색이 덮어써진다 (painter's algorithm). 카메라가 돌면 앞뒤가 뒤바뀌어 *뒤 물체가 앞 물체를 덮는* 깨짐이 생긴다.

**Depth test** = 새 fragment 를 그리기 전, *그 픽셀의 기존 깊이값* 과 *새 fragment 의 깊이값* 을 비교한다. 비교 통과 시에만 color buffer + depth buffer 를 갱신.

```
fragment 생성 → depth 비교 (glDepthFunc 기준) → 통과? → color/depth 기록
                                              → 실패? → fragment 폐기
```

→ 그리는 순서와 무관하게 *항상 카메라에 가까운 면이 보인다*.

---

## 3. Depth 관련 GL 호출 — 프로젝트 매핑

[context.cpp](../../src/context/context.cpp) 의 depth 관련 호출:

| GL 호출 | 역할 | 프로젝트 위치 |
|---------|------|--------------|
| `glEnable(GL_DEPTH_TEST)` | depth test *켜기* — 끄면 painter's algorithm 으로 회귀 | [context.cpp Render](../../src/context/context.cpp) |
| `glDisable(GL_DEPTH_TEST)` | depth test *끄기* — §6 참조 | (주석 처리) |
| `glClear(GL_DEPTH_BUFFER_BIT)` | depth buffer 를 `glClearDepth` 값으로 초기화 | `Render()` 매 프레임 첫줄 |
| `glClearDepth(1.0f)` | clear 시 채울 깊이값 — 기본 1.0 (= 가장 멀리) | (주석 — 기본값 사용) |
| `glDepthFunc(func)` | depth *비교 연산자* 선택 (§4) | `Render()` — ImGui Combo 가 고른 값 |
| `glDepthMask(GL_FALSE)` | depth buffer *쓰기 막기* — 테스트는 하되 기록 안 함 (반투명 렌더 등) | (주석 처리) |

> **State-setting** 함수들이다 ([GLState.md](GLState.md) §1) — 한 번 호출하면 다음 draw 들이 그 상태를 본다.
> `glEnable(GL_DEPTH_TEST)` 는 *글로벌* 컨텍스트 상태.

---

## 4. Depth 비교 연산자 — `glDepthFunc`

깊이값 범위는 `[0, 1]` — **0 = 가장 가까움, 1 = 가장 멀리**. `glDepthFunc` 는 "새 fragment 가 *어떤 조건* 일 때 통과시킬지" 를 정한다.

| 인덱스 | 값 | 의미 |
|--------|-----|------|
| 0 | `GL_ALWAYS` | 항상 통과 (depth test 무력화 효과) |
| 1 | `GL_NEVER` | 항상 실패 (아무것도 안 그려짐) |
| 2 | `GL_LESS` | 새 깊이 < 기존 → 통과 *(기본값)* — 더 가까우면 그림 |
| 3 | `GL_LEQUAL` | 같거나 가까우면 통과 |
| 4 | `GL_GREATER` | 더 멀면 통과 |
| 5 | `GL_GEQUAL` | 같거나 멀면 통과 |
| 6 | `GL_EQUAL` | 깊이가 정확히 같을 때만 |
| 7 | `GL_NOTEQUAL` | 깊이가 다를 때만 |

기본값 `GL_LESS` — "1(가장 멀리)보다 더 작은(가까운) 것을 통과시킨다". 즉 *가까운 면이 이긴다*.

> 프로젝트는 이 8개를 ImGui Combo 로 런타임 전환 가능하게 해둠 — `DEPTH_FUNC_LABELS[]` (라벨) 와 `DEPTH_FUNC[]` (GL enum) 의 *인덱스 순서가 1:1 동일* 해야 한다. 둘 중 하나만 바뀌면 라벨과 실제 동작이 어긋남.

---

## 5. Depth 값의 비선형 분포 — z-fighting

Perspective projection 은 깊이값을 `[0, 1]` 로 정규화하면서 `w` 로 나눈다. 그 결과 정규화된 z 는 **`1/z` 꼴** 로 분포 — *가까운 곳은 정밀, 먼 곳은 듬성듬성*.

```
실제 거리:  near ──────────────────────────── far
정규화 z :  0   0.5  0.8  0.9 0.95 ......... 1.0
            ↑ 가까운 쪽에 정밀도 집중      ↑ 먼 쪽은 값 차이 미미
```

### z-fighting

먼 거리의 두 면은 정규화 z 값이 *거의 같아* — depth test 가 어느 게 앞인지 판정 못 해 픽셀이 *깜빡이며 다투는* 현상.

**예방** (context.cpp 주석에서):
- 면과 면을 *너무 가깝게 겹치지* 않기.
- `near` 평면을 *너무 작게* 잡지 않기 — near 가 작을수록 `1/z` 곡선이 가팔라져 먼 쪽 정밀도가 더 망가짐.
- 더 정밀한 depth buffer (24→32-bit) 사용.

> [Camera::GetProjMatrix](../../src/object/camera.h) 의 `NearPlane` / `FarPlane` 가 이 분포를 결정. `NearPlane` 을 0.001 같이 극단적으로 작게 두면 먼 물체가 z-fighting 에 취약해진다.

---

## 6. Depth Test 를 *끄는* 경우

`glEnable(GL_DEPTH_TEST)` 가 기본이지만, *의도적으로 꺼야* 하는 상황이 있다 — depth 와 무관하게 *항상 앞* 또는 *항상 뒤* 로 그려야 할 때.

| 상황 | 이유 |
|------|------|
| **ImGui / HUD / UI** | UI 는 3D 씬 *위에 항상* 떠야 함. depth 비교 대상이 아님 — 그래서 ImGui 렌더 구간은 depth test off |
| **Skybox** | 항상 *가장 뒤* — `GL_LEQUAL` + depth=1.0 트릭 또는 test off |
| **반투명 (blend) 객체** | depth *test* 는 하되 *write* 는 막음 (`glDepthMask(GL_FALSE)`) — 뒤 객체가 비쳐 보이도록 |

> 프로젝트 주석: *"Depth Test 를 꺼야 하는 상황은? → ImGui 를 사용할 때."* 정확하다. UI 는 3D 깊이 순서와 별개 레이어.

---

## 7. 다음 단계 — Framebuffer Object (FBO)

여기까지가 *default framebuffer* (윈도우 화면) + depth buffer 의 기초. 본격적인 framebuffer 학습은 **off-screen 렌더링**:

```
씬을 화면이 아닌 *내가 만든 FBO* 에 그림
  → 그 결과(color/depth)를 텍스처로 받음
  → 그 텍스처를 다시 화면에 그리거나 후처리
```

활용 예:
- **그림자 맵** — light 시점에서 depth 만 FBO 에 렌더 → 그 depth 텍스처로 그림자 판정
- **포스트 프로세싱** — 씬을 FBO 텍스처로 받아 blur / bloom / tone-mapping
- **mirror / portal** — 다른 시점 렌더를 텍스처로

핵심 GL 객체: `glGenFramebuffers` / `glBindFramebuffer` / `glFramebufferTexture2D` / `glCheckFramebufferStatus`.

> depth test 가 *default framebuffer 의 depth buffer* 를 다뤘다면, FBO 단계에선 *내가 depth attachment 를 직접 만들어 붙인다*. depth buffer 가 "자동으로 거기 있는 것" 이 아니라 *framebuffer 의 한 attachment* 임을 그때 체감하게 된다.

---

## 참고

- GL state-setting / state-using 구분: [GLState.md](GLState.md)
- depth 관련 호출은 모두 state-setting — 호출 순서가 결과를 좌우.
- 프로젝트 코드: [context.cpp](../../src/context/context.cpp) `Render()` 의 `glClear` / `glEnable(GL_DEPTH_TEST)` / `glDepthFunc`.
