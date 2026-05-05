# Migration Plan — `main` → `newEnv` 단계별 구현 로드맵

> 본 문서는 `newEnv` 브랜치의 모듈 구조(`src/<module>/`, `SJH::` alias, `diagnostics` 등)로 `main` 브랜치의 OpenGL 학습 구현을 단계적으로 이식하는 계획서다.
>
> **핸드오프 대상**: 다른 Claude Agent 또는 미래의 본인. 본 계획은 [.claude/architecture.md](../.claude/architecture.md)의 컨벤션을 *전제*로 한다 — 새 모듈 추가 절차, 클래스 디자인 패턴, PUBLIC/PRIVATE 결정 트리는 그쪽 참조.

## 0. 배경

| 브랜치 | 구조 | 상태 |
|---|---|---|
| `main` | flat `src/<file>.cpp` (글로벌 namespace, 인라인 SPDLOG_ERROR) | 학습 끝 시점 — 사각형 그리기 + 멀티 텍스처 + 이미지 블렌딩 동작 |
| `newEnv` (현재) | 모듈별 디렉토리 + `namespace SJH` + `diagnostics` 분리 | 빌드 시스템·컨벤션 정비 끝, **구현 미완** |

**목표**: `newEnv`가 `main`과 *기능적으로 동등*한 시점까지 도달 (멀티 텍스처 사각형 렌더링).

## 1. 모듈별 갭 분석

| 모듈 | main 구현 요지 | newEnv 상태 | 작업량 |
|---|---|---|---|
| `common` | `LoadTextFile()`, `CLASS_PTR` 매크로 | ✅ 활성 (개선됨) | — |
| `shader` | `CreateFromFile()`, `LoadFile()` 인라인 검증 | ✅ 활성 (`TryLoadFile` + diagnostics) | — |
| `diagnostics` | (main에 없음) | ✅ 활성 (newEnv 신설) | — |
| `buffer` | `CreateWithData(type, usage, data, size)`, `Bind()` | placeholder | **소** |
| `vertex_layout` | `Create()`, `SetAttrib(...)`, `Bind()` | placeholder (`layout/`) | **소** |
| `program` | `Create(vector<ShaderPtr>)`, `Link()` 인라인 검증, `Use()` | placeholder | **소-중** |
| `image` | stb_image 로드, `Create()`, `SetCheckImage()` | **모듈 부재** | **중** |
| `texture` | `CreateFromImage()`, `SetFilter/Wrap`, mipmap | **모듈 부재** | **중** |
| `context` | 모든 모듈 통합 + 멀티 텍스처 + uniform 바인딩 | placeholder | **대** |

### 자원 현황 갭
| 자원 | main 경로 | newEnv 경로 | 작업 |
|---|---|---|---|
| 셰이더 | `./shader/texture.vs/.fs` | `resources/shader/simple.vs/.fs` | Phase 4에서 셰이더 코드 갱신 또는 추가 |
| 이미지 | `./image/container.jpg`, `./image/awesomeface.png` | `resources/textures/` (디렉토리 없음) | Phase 1-C에서 자원 배치 결정 |

### newEnv가 main 대비 추가/개선한 항목 (유지)
- 모듈별 `CMakeLists.txt` + `SJH::*` alias
- `namespace SJH`로 글로벌 오염 방지
- `Try*` prefix로 fallible 메서드 표시
- `diagnostics` 모듈로 GL 진단 패턴 중앙화
- vcpkg manifest mode + 명시적 의존성 link

## 2. 의존성 그래프

```
common (✓)         diagnostics (✓)
   │                    │
   └────────┬───────────┘
            ↓
        shader (✓)
            ↓
        program (Phase 2)

buffer (Phase 1-A) ────────┐
vertex_layout (Phase 1-B) ─┤
image (Phase 1-C) ─────────┤
        ↓                  │
   texture (Phase 3) ──────┤
                           ↓
                      context (Phase 4)
                           ↓
                  app/main.cpp 통합 (Phase 5)
```

## 3. Phase 계획

각 Phase는 **빌드 가능 + 회귀 없음**을 완료 조건으로 한다 (`cmake --build build_Darwin`).

---

### Phase 1: 독립 GL 자원 wrapper — 병렬 작업 가능

세 모듈 모두 서로 독립이므로 동시 진행 가능. 각 작업은 [.claude/architecture.md §7](../.claude/architecture.md) 체크리스트 따름.

#### Phase 1-A: `buffer` 활성화
- [ ] [src/CMakeLists.txt](../src/CMakeLists.txt): `# add_subdirectory(buffer)` 주석 해제
- [ ] [src/buffer/CMakeLists.txt](../src/buffer/CMakeLists.txt): 의존성 추가
  ```cmake
  target_link_libraries(sjhopengl_buffer
      PUBLIC  SJH::common glad::glad
      PRIVATE SJH::diagnostics
  )
  ```
- [ ] [src/buffer/buffer.h](../src/buffer/buffer.h): main의 `Buffer` 클래스 이식 + `namespace SJH` 감싸기 + `CLASS_PTR(Buffer)` 적용
- [ ] [src/buffer/buffer.cpp](../src/buffer/buffer.cpp): main 구현 이식. 주요 GL 호출:
  - `glGenBuffers` / `glBufferData` / `glBindBuffer` / `glDeleteBuffers`
- [ ] **검토 사항**: `Init()` 메서드명 — main은 `Init()`, 컨벤션은 `Try*` prefix 권장 → `TryInit()` 권장
- [ ] **선택**: `glBufferData` 호출을 `SJH_GL_CHECK(...)`로 감싸기 (디버그 빌드 진단 강화)
- [ ] 빌드 검증

#### Phase 1-B: `layout` (vertex_layout) 활성화
- [ ] [src/CMakeLists.txt](../src/CMakeLists.txt): `# add_subdirectory(layout)` 주석 해제
- [ ] [src/layout/CMakeLists.txt](../src/layout/CMakeLists.txt): 의존성 추가 (`buffer`와 동일 패턴)
- [ ] [src/layout/vertex_layout.h](../src/layout/vertex_layout.h): main의 `VertexLayout` 이식 + `namespace SJH`
- [ ] [src/layout/vertex_layout.cpp](../src/layout/vertex_layout.cpp): main 구현 이식. 주요 GL 호출:
  - `glGenVertexArrays` / `glBindVertexArray` / `glDeleteVertexArrays`
  - `glEnableVertexAttribArray` / `glVertexAttribPointer` / `glDisableVertexAttribArray`
- [ ] **참고**: main의 `Init()`은 `void` (실패 케이스 없음). 그대로 유지 — 컨벤션 위반 아님 (Try*는 fallible할 때만).
- [ ] 빌드 검증

#### Phase 1-C: `image` 모듈 신설
- [ ] `src/image/` 디렉토리 생성
- [ ] `src/image/CMakeLists.txt` 신설 ([architecture.md §2 template](../.claude/architecture.md))
  - `target_include_directories(... PRIVATE ${Stb_INCLUDE_DIR})` — stb는 헤더만 vcpkg에서 받음
  - `PUBLIC SJH::common`, `PRIVATE SJH::diagnostics` (현재 image는 stb 에러를 spdlog로 출력하므로 spdlog도 따라옴)
- [ ] `src/image/image.h`: main의 `Image` 클래스 이식 + `namespace SJH`
- [ ] `src/image/image.cpp`:
  - `#define STB_IMAGE_IMPLEMENTATION`은 **이 파일에만** (다중 정의 금지)
  - `#include <stb/stb_image.h>` (vcpkg는 `stb` 디렉토리 prefix 사용)
- [ ] [src/CMakeLists.txt](../src/CMakeLists.txt): `add_subdirectory(image)` 추가
- [ ] [.claude/MEMORY.md](../.claude/MEMORY.md), [.claude/architecture.md](../.claude/architecture.md) 모듈 인벤토리 갱신
- [ ] **개선 후보 (선택)**: main의 `mData`는 raw `uint8_t*` (malloc/stbi_image_free). `std::unique_ptr<uint8_t, decltype(&stbi_image_free)>` 또는 `std::vector<uint8_t>`로 RAII화 가능 — 단 `Allocate()`(malloc)와 `LoadWithStb()`(stbi_load) 두 경로의 deleter가 다르다는 점 주의.
- [ ] 빌드 검증

**Phase 1 완료 시점**: 세 모듈이 빌드되지만 아직 어디서도 사용되지 않음. `app/main.cpp`은 변화 없음.

---

### Phase 2: `program` 활성화

shader에 의존. 가장 큰 가치는 `diagnostics::CheckProgramLink` 적용 사례 확립.

- [ ] [src/CMakeLists.txt](../src/CMakeLists.txt): `# add_subdirectory(program)` 주석 해제
- [ ] [src/program/CMakeLists.txt](../src/program/CMakeLists.txt):
  ```cmake
  target_link_libraries(sjhopengl_program
      PUBLIC  SJH::common SJH::shader glad::glad
      PRIVATE SJH::diagnostics
  )
  ```
- [ ] [src/program/program.h](../src/program/program.h): main 이식 + `namespace SJH`
- [ ] [src/program/program.cpp](../src/program/program.cpp): main의 `Link()` 마이그레이션
  - main의 1024 고정 버퍼 + `SPDLOG_ERROR` 검증 → **한 줄로 대체**:
    ```cpp
    glLinkProgram(mProgram);
    return diagnostics::GLObjectLog::CheckProgramLink(mProgram, "<program tag>");
    ```
  - shader 마이그레이션과 같은 패턴 ([src/shader/shader.cpp:36](../src/shader/shader.cpp#L36) 참고)
- [ ] **컨벤션 적용**: `Link()` → `TryLink()` 권장 (fallible)
- [ ] 빌드 검증

---

### Phase 3: `texture` 모듈 신설

image에 의존.

- [ ] `src/texture/` 디렉토리 생성
- [ ] `src/texture/CMakeLists.txt`:
  ```cmake
  target_link_libraries(sjhopengl_texture
      PUBLIC  SJH::common SJH::image glad::glad
      PRIVATE SJH::diagnostics
  )
  ```
- [ ] `src/texture/texture.h/cpp`: main 이식 + `namespace SJH`
- [ ] **선택**: `glTexImage2D`, `glGenerateMipmap`을 `SJH_GL_CHECK`로 감싸기
- [ ] [src/CMakeLists.txt](../src/CMakeLists.txt) + 메모리 문서 갱신
- [ ] 빌드 검증

---

### Phase 4: `context` 모듈 — 통합 마일스톤 (가장 큼)

위 모든 모듈 의존. main의 `Context::Init()` 통합 로직 이식.

#### 4-1. CMake / 헤더
- [ ] [src/context/CMakeLists.txt](../src/context/CMakeLists.txt):
  ```cmake
  target_link_libraries(sjhopengl_context
      PUBLIC  SJH::common SJH::shader SJH::program SJH::buffer
              SJH::layout SJH::image SJH::texture glad::glad
      PRIVATE SJH::diagnostics
  )
  ```
  - **주의**: `glfw`는 link 안 함 — Context는 GL 자원 관리만, 윈도우/입력은 `app/`이 담당.
- [ ] `context.h`: main의 `Context` 이식, 멤버 UPtr들 (`ProgramUPtr`, `VertexLayoutUPtr`, `BufferUPtr`, `TextureUPtr` × 2)

#### 4-2. 자원 경로 결정 (블로커 — 진행 전 확정 필요)
main은 `./shader/texture.vs`, `./image/container.jpg`, `./image/awesomeface.png` 사용. 현재 newEnv 자원 상태:
- ✅ [resources/shader/simple.vs](../resources/shader/simple.vs), [resources/shader/simple.fs](../resources/shader/simple.fs) 존재
- ❌ 텍스처용 셰이더 (`texture.vs/.fs`) **없음** — 신규 작성 또는 main에서 가져와야 함
- ❌ 이미지 자원 (`container.jpg`, `awesomeface.png`) **없음** — 추가 필요 (Phase 4의 sub-task)

**결정 필요**:
- (a) `resources/shader/texture.vs/.fs` 추가 (main에서 이식 또는 새로 작성)
- (b) `resources/textures/container.jpg`, `awesomeface.png` 배치
- (c) 작업 디렉토리 — 현재 main은 상대 경로 (`./shader/...`) 사용, newEnv도 동일하게 할지 `${CMAKE_SOURCE_DIR}/resources/...` 절대 경로로 갈지

#### 4-3. context.cpp 이식
- [ ] main 구현 이식 + `namespace SJH`
- [ ] 자원 경로를 위 결정에 맞게 갱신
- [ ] `SPDLOG_INFO` 호출은 그대로 유지 (비-GL 정보성 로그)
- [ ] **선택**: 그리기 호출(`glDrawElements`)을 `SJH_GL_CHECK`로, draw 직전 디버그 빌드만 `CheckProgramValidate` 적용

#### 4-4. 빌드 + 단위 검증
- [ ] 빌드만 통과시킴 (아직 main.cpp 미통합 → 실행은 Phase 5)

---

### Phase 5: `app/main.cpp` 통합 + `GLDebug::Init()` — **시각적 마일스톤**

- [ ] [app/CMakeLists.txt](../app/CMakeLists.txt): 주석된 `SJH::*` 모듈 link 활성화
  ```cmake
  target_link_libraries(${PROJECT_NAME} PRIVATE
      spdlog::spdlog glfw glad::glad fmt::fmt
      SJH::shader SJH::program SJH::buffer SJH::common
      SJH::context SJH::layout SJH::image SJH::texture
      SJH::diagnostics    # 신규
  )
  ```
- [ ] [app/main.cpp](../app/main.cpp): glad 로딩 직후 한 줄 추가
  ```cpp
  if (!gladLoadGLLoader(...)) { /*...*/ }
  SJH::diagnostics::GLDebug::Init();   // ← 추가
  ```
- [ ] 메인 루프 안의 `Render()` (현재 빈 함수)를 Context로 교체:
  ```cpp
  auto context = SJH::Context::Create();
  if (!context) return -1;
  while (!glfwWindowShouldClose(window)) {
      context->Render();
      glfwSwapBuffers(window);
      glfwPollEvents();
  }
  ```
- [ ] 실행 검증: `./build_Darwin/OpenGL-With-CMake` — 멀티 텍스처 사각형이 화면에 나옴
- [ ] **회귀 검사**: 키보드 스페이스 종료, 윈도우 리사이즈 콜백 정상 동작 확인

**완료 = main 브랜치와 기능적으로 동등**.

---

### Phase 6: 점진적 개선 (선택, 우선순위 낮음)

마일스톤 후 여유 있을 때 진행.

- [ ] `Image`의 raw `uint8_t*` → RAII (`unique_ptr` + custom deleter, 또는 `vector<uint8_t>`)
- [ ] 그리기 호출 전반에 `SJH_GL_CHECK` 적용 (디버그 빌드만)
- [ ] `Context::Render`에서 디버그 빌드 한정 `CheckProgramValidate`
- [ ] `common.cpp`의 `spdlog::error` 직접 사용 → 일반 로깅 facade 도입 여부 재검토 (현재는 YAGNI로 보류)
- [ ] (만약 `glfw` 분리 모듈로 추출 가치가 있으면) `SJH::window` 모듈로 windowing 추상화

---

## 4. 진행 순서 요약

```
Phase 1-A buffer  ─┐
Phase 1-B layout  ─┤── 병렬 작업 (독립 모듈)
Phase 1-C image   ─┘
        ↓
Phase 2 program       ← shader 의존
        ↓
Phase 3 texture       ← image 의존
        ↓
Phase 4 context       ← 모든 모듈 의존, 자원 경로 결정 필요
        ↓
Phase 5 app/main.cpp  ← 시각적 마일스톤 (main 동등성 달성)
        ↓
Phase 6 개선 (선택)
```

## 5. Phase별 추정 난이도

| Phase | 난이도 | 핵심 위험 |
|---|---|---|
| 1-A buffer | 낮음 | 거의 없음 — 작은 GL wrapper |
| 1-B layout | 낮음 | 거의 없음 |
| 1-C image | 중 | stb `STB_IMAGE_IMPLEMENTATION` 위치, 메모리 관리(malloc vs stbi_image_free) |
| 2 program | 낮음 | shader 의존 — 이미 활성이라 안전 |
| 3 texture | 중 | image 의존 + 채널 수 분기(`GL_RED/RG/RGB/RGBA`) |
| 4 context | **높음** | 자원 경로 결정 + 다수 모듈 통합 + 셰이더/텍스처 자원 부재 |
| 5 main.cpp | 중 | 처음 실행 시 시각 디버깅 필요 가능성 |
| 6 개선 | 낮음 | 회귀 없도록 단계별 진행 |

## 6. Phase 진행 시 일반 체크리스트

각 Phase 종료 시:

- [ ] `cmake --build build_Darwin` 통과 (warning 없이)
- [ ] [.claude/MEMORY.md](../.claude/MEMORY.md) 모듈 인벤토리 갱신 (placeholder → 활성)
- [ ] [.claude/architecture.md](../.claude/architecture.md) §5 모듈 인벤토리 동기화
- [ ] (Phase 1-C 등 새 모듈 추가 시) `architecture.md`에 모듈 항목 추가
- [ ] git commit (작업 단위로 분리, 메시지 컨벤션은 [git log](https://github.com) 참조)

## 7. 핸드오프 시 즉시 알아야 할 것

다른 Claude Agent가 이 plan을 받아 진행할 때 가장 먼저 인지할 사항:

1. **컨벤션은 [.claude/architecture.md](../.claude/architecture.md)이 정답** — 본 plan은 *언제 무엇을* 다루지만 *어떻게* 만드는지는 architecture.md.
2. **이 코드베이스는 예외를 안 던진다** — `Try*` 메서드는 bool, 팩토리는 nullptr 반환.
3. **Phase 4의 자원 경로 결정**은 사용자/팀이 내려야 함 — 임의로 진행 금지.
4. **`diagnostics` 모듈은 GL 전용** — 비-GL 로깅은 spdlog 직접 사용 (architecture.md §6 책임 경계 참조).
5. **placeholder 파일** (`namespace SJH {}` 빈 파일)은 의도된 신호 — 본격 구현 시점이 아니라는 뜻.
6. **macOS는 GL 3.3 강제** — 4.3+ 기능 사용 시 컴파일+런타임 이중 가드 필수.

## 8. 참조

- [.claude/architecture.md](../.claude/architecture.md) — 클래스/모듈 디자인, PUBLIC/PRIVATE 정책, diagnostics 상세
- [.claude/build-system.md](../.claude/build-system.md) — CMake 빌드 시스템 상세
- [.claude/MEMORY.md](../.claude/MEMORY.md) — 프로젝트 인덱스
- `git log main` — main 브랜치 커밋 history (각 Phase가 어떤 main 커밋과 대응되는지 추적 가능)
