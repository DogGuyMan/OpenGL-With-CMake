# OpenGL State — 노트

OpenGL 은 거대한 **상태 머신(state machine)**. 거의 모든 `gl*` 호출은 *컨텍스트에 저장된 상태를 바꾸거나*, *그 상태를 읽어 결과를 낸다*. 이 노트는 본 프로젝트에서 *어떤 코드가 어떤 상태를* 바꾸는지 정리한다.

---

## 1. State-setting vs State-using — 두 부류

모든 `gl*` 함수는 둘 중 하나다.

| 부류 | 하는 일 | 예 |
|------|---------|-----|
| **State-setting** | 컨텍스트의 *상태를 바꾼다* — 이후 호출들이 그 상태를 본다 | `glBindXxx`, `glGenXxx`, `glActiveTexture`, `glEnable`/`glDisable`, `glUseProgram`, `glViewport`, `glTexImage2D`, `glVertexAttribPointer`, `glClearColor`, `glDepthFunc` |
| **State-using** | 현재 상태를 *읽어서* 결과(픽셀 등)를 낸다 — 상태 자체는 안 바꿈 | `glDrawElements`, `glDrawArrays`, `glClear` |

### ⚠️ 흔한 오해 — `glDrawElements` 는 state 를 바꾸지 않는다

`glDrawElements` / `glDrawArrays` 는 *현재 바인딩된 VAO·program·texture 를 소비해 픽셀을 만들 뿐*, 바인딩 상태의 어떤 필드도 바꾸지 않는다. "draw call 이후 무슨 state 가 바뀌나" 를 적을 게 — 없다.

`glClear` 도 마찬가지로 state-using — *프레임버퍼의 픽셀 내용* 은 바꾸지만 GL 의 *바인딩 상태* 는 안 건드린다.

### App 메인 루프의 분담

[app/main.cpp](../../app/main.cpp) 주석의 표현 그대로:
> "세팅 함수들은 어딘가 Context 에 데이터를 저장한다. State-setting 은 Context 에 저장, State-using 은 저장된 State 를 이용."

---

## 2. State 변경의 *스코프* — 어디에 저장되는가

state-setting 호출이라도 *변경 범위* 가 다르다. 이것을 모르면 "분명 바인딩했는데 안 보인다" 류의 버그에 빠진다.

| `gl*` 호출 | 변경 스코프 | 의미 |
|-----------|-------------|------|
| `glBindVertexArray` | **글로벌** 컨텍스트 | 현재 VAO 선택 |
| `glBindBuffer(GL_ARRAY_BUFFER)` | **글로벌** | VAO 무관. 단 `glVertexAttribPointer` 가 *이 시점의 바인딩* 을 캡쳐 |
| `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER)` | **현재 VAO 안** | VAO 마다 별도. VAO=0 이면 항상 0 |
| `glVertexAttribPointer` / `glEnableVertexAttribArray` | **현재 VAO 안** | attrib 슬롯 + 캡쳐된 VBO |
| `glActiveTexture` | **글로벌** | 텍스처 유닛 셀렉터 |
| `glBindTexture` | **현재 active 유닛 안** | `glActiveTexture` 가 고른 유닛에 바인딩 |
| `glUseProgram` | **글로벌** | 현재 program 선택 |
| `glUniform*` | **현재 use 중인 program 안** | program 마다 별도. use 안 하면 `GL_INVALID_OPERATION` |
| `glEnable`/`glDepthFunc`/`glViewport`/`glClearColor` | **글로벌** | 전역 렌더 상태 |

> **핵심**: `glVertexAttribPointer` 는 *호출 시점의 `GL_ARRAY_BUFFER` 바인딩* 을 VAO 에 캡쳐한다. 그래서 *VBO 바인딩 → attrib 설정* 순서가 중요하고, *VAO 가 바인딩된 상태* 에서 해야 한다.

---

## 3. 프로젝트의 state 변경 지점 — 매핑

GL state 스냅샷의 각 필드 ← 그것을 바꾸는 `gl*` 호출 ← 본 프로젝트의 코드 위치.

| 스냅샷 필드 | state-setting 호출 | 프로젝트 위치 | 호출 경로 |
|------------|-------------------|--------------|----------|
| `vao` | `glGenVertexArrays` → `glBindVertexArray` | `src/layout/vertex_layout.cpp` | `VertexLayout::Create()` |
| `array_buffer` | `glGenBuffers` → `glBindBuffer(GL_ARRAY_BUFFER)` → `glBufferData` | `src/buffer/buffer.cpp` | `Buffer::CreateWithData(GL_ARRAY_BUFFER,…)` |
| `element_buffer` | `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER)` → `glBufferData` | `src/buffer/buffer.cpp` | `Buffer::CreateWithData(GL_ELEMENT_ARRAY_BUFFER,…)` — *현재 VAO 에 기록* |
| `attrib[N]` | `glEnableVertexAttribArray` → `glVertexAttribPointer` | `src/layout/vertex_layout.cpp` | `VertexLayout::TrySetAttrib()` |
| `tex_2d[unit]` | `glGenTextures` → `glBindTexture` → `glTexParameteri` → `glTexImage2D` | `src/resource_registry/texture.cpp` | `Texture::CreateTexture()` |
| `active_texture` | `glActiveTexture` | `src/material/material.cpp` 등 | `Material::Bind()` |
| `program` | `glUseProgram` | `src/program/program.cpp` | `Program::Use()` |
| `clear_color` | `glClearColor` | `src/context/context.cpp` | `Context::Init()` |
| `viewport` | `glViewport` | `src/context/context.cpp` | `Context::Reshape()` |
| `depth_func` | `glDepthFunc` | `src/context/context.cpp` | `Context::Render()` |

### state 변경의 *시작점*

"전부 0 인 초기 상태 → 채워진 상태" 의 시작점은 **`Context::Init()`**. Init 이 위 팩토리들(`VertexLayout::Create`, `Buffer::CreateWithData`, `Texture::CreateTexture`, `Program`)을 순차 호출하며 각자 자기 영역의 state 를 켠다.

`glDrawElements` 는 이 그림에 없다 — state-using 이므로.

---

## 4. State 를 *검증* 하는 인프라 — 주석이 아니라 코드로

"이 `gl*` 호출이 무슨 state 를 바꾸나" 를 *주석* 으로 적으면 코드와 어긋날 수 있다. 본 프로젝트는 이를 *실행되는 코드* 로 한다.

### (a) 호출별 진단 — `Diagnostics::GLDebug::CheckGL*`
`buffer.cpp` / `vertex_layout.cpp` 가 각 `gl*` 호출 *직후* `CheckGL*` 를 부른다 — "이 호출이 에러 없이 의도한 state 를 건드렸나" 의 검증.

### (b) state 캡쳐 + diff — `diagnostics/gl_state_fields.cpp`, `gl_validate.cpp`
GL state 스냅샷을 *캡쳐* 하고, 두 스냅샷을 *diff* 하면 "어떤 코드 구간이 어떤 state 를 바꿨나" 가 자동으로 드러난다.

**권장 디버깅법**: 의심 구간 전후로 `CaptureGLState()` → diff. "이 코드가 무슨 state 를 바꿨나" 가 *증명* 된다.

---

## 5. 교훈 — *스코프 무지* 가 만든 단색 회귀

> 증상: 텍스처 대신 단색 사각형. 원인 추적에 여러 라운드 소요.

스냅샷에 이런 줄이 있다:
```
element_buffer: 0  (note: EBO state is per-VAO; with VAO=0, this is always 0)
```

`GL_ELEMENT_ARRAY_BUFFER` 바인딩은 **현재 VAO 안** 에 저장된다 (§2). VAO=0 (default VAO) 상태에서 조회하면 항상 0 — *EBO 를 바인딩 안 한 게 아니라, 다른 VAO 의 상태를 보고 있는 것*.

마찬가지로 vertex attribute 도 VAO 안에 저장 — `Render()` 에서 VAO 를 다시 바인딩하지 않으면 attribute 가 default value `(0,0,0,1)` 로 떨어져, 모든 fragment 가 같은 texCoord 를 써서 *한 텍셀만 단색* 으로 그려진다.

**규칙**:
- draw 전에 *반드시* 의도한 VAO 를 `Bind()`.
- state 가 "안 보인다" 싶으면 — 안 바꾼 게 아니라 *다른 스코프를 보고 있는지* 의심.
- 추측 대신 `CaptureGLState()` 로 *실제 스코프의 값* 을 확인.

---

## 참고

- 호출별 진단 함수 목록: [.claude/architecture.md §6](../../.claude/architecture.md)
- state-setting / state-using 분류는 OpenGL spec 의 근본 모델 — 모든 GL 학습의 출발점.
