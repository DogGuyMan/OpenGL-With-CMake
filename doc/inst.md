# 📋 프롬프트 — Mesh/Shader 정합성 진단 모듈 구축

> 이 프롬프트는 OpenGL-With-CMake 프로젝트 (C++17 + CMake + vcpkg, macOS arm64 / Windows x64) 의 라이팅·메시·셰이더 마이그레이션 과정에서 *반복적으로 등장한* 6가지 버그 카테고리를 자동 탐지하는 **진단 모듈** 을 구축하는 작업이다.

---

## 1. 작업 범위 — Scope

### IN scope
- `src/diagnostics/` 하위에 **진단 전용 헤더/소스 추가**
- 기존 `Diagnostics::GLStateLog` 와 같은 네임스페이스/패턴 유지 — 새 진단도 `Diagnostics::` 네임스페이스
- 호출자 (Context::Init / Context::Render) 에서 **명시적으로** 호출하면 동작 (전역 등록 X)
- spdlog 출력으로 결과 보고 — 진단 실패는 `warning`, 치명적 mismatch 는 `error`

### OUT of scope (이번 작업에서 건드리지 말 것)
- ❌ 본 코드 (mesh.cpp / lighting.fs / context.cpp 등) 의 비즈니스 로직 수정
- ❌ 기존 `Diagnostics::GLStateLog` 모듈 리팩토링
- ❌ 새 셰이더 / 새 광원 타입 추가
- ❌ FLIP 골든 이미지 비교 (CLAUDE.md §B 의 별도 작업 — Phase 2 로 분리)
- ❌ Mesa llvmpipe 통합 (별도 환경 작업)

---

## 2. 탐지해야 할 버그 카테고리 — 실제 발생 사례 6종

각 카테고리마다 *과거에 발생한 정확한 사례* 와 *진단 함수가 잡아야 할 조건* 을 명시한다.

### Cat A — VAO index out-of-range / degenerate triangle
**실 사례**: `mesh.cpp` 의 `indices[]` 에서 right/bottom/top 면 인덱스가 다른 면의 정점을 잘못 참조 (예: `i6: 20, 2, 1, 2, 0, 3` 는 i1 인덱스를 그대로 복사).

**진단 조건**:
- 모든 index ∈ `[0, vertexCount)` 여야 함
- 같은 triangle 의 세 vertex index 가 서로 달라야 함 (degenerate = zero-area)
- 같은 triangle 이 다른 위치에서 *완전 중복* 으로 나타나면 warning

### Cat B — Vertex attribute layout 불일치 (VAO ↔ VS)
**실 사례**: `Vertex { vec3 pos; vec3 normal; vec2 uv; }` (3 attribs) vs lighting.vs `layout(location = 0/1/2/3)` (4 attribs) — location 2 가 mesh 에선 uv(vec2), shader 에선 aColor(vec3) 로 어긋남.

**진단 조건**:
- VS 가 활성화한 location 마다, VAO 에 `glGetVertexAttrib(loc, GL_VERTEX_ATTRIB_ARRAY_ENABLED)` 가 true 여야 함
- `glGetVertexAttrib(loc, GL_VERTEX_ATTRIB_ARRAY_SIZE)` 가 VS 의 `vec3/vec2` 와 컴포넌트 수 일치
- VS 가 안 쓰는데 VAO 가 enable 한 location 도 warning (낭비)

### Cat C — Uniform 누락 / dead uniform set
**실 사례**: `tex0`, `tex1`, `light.attenuation` 등 셰이더에서 제거된 uniform 에 C++ 가 SetInt/SetVec3 호출 → `glGetUniformLocation` 이 `-1` 반환.

**진단 조건**:
- C++ 가 호출한 모든 uniform 이름이 해당 program 에 *active uniform* 으로 존재
- 셰이더에 *declared* 이지만 C++ 가 한 번도 set 안 한 uniform 도 warning (CPU 미송신 → GL default 0)

### Cat D — Sampler unit ↔ glActiveTexture 불일치
**실 사례**: `Uniforms::SetInt(prog, "material.specular", 3)` 만 호출했는데 `glActiveTexture(GL_TEXTURE3)` + `Bind()` 누락 → unit 3 에 텍스처 없음 → black sampling.

**진단 조건**:
- 모든 sampler2D uniform 이 N 으로 set 됐다면, `glGetIntegeri_v(GL_TEXTURE_BINDING_2D, N)` ≠ 0
- 같은 texture object id 가 *서로 다른* sampler unit 에 동시에 bound 인 경우 정보 출력 (intentional 일 수도 있으므로 info 레벨)

### Cat E — GL error 매 draw 후 catch
**실 사례**: macOS Metal 호환 레이어가 일부 `glGet*` 에서 `GL_INVALID_OPERATION (0x502)` 반환.

**진단 조건**:
- `glGetError()` 를 draw call 직후 호출 → non-zero 이면 enum 이름 (`GL_INVALID_OPERATION`/`GL_INVALID_ENUM`/`GL_INVALID_VALUE`/`GL_OUT_OF_MEMORY`/`GL_INVALID_FRAMEBUFFER_OPERATION`) 로 변환해 spdlog 출력
- 매 프레임 spam 방지 — 같은 에러 코드는 *프레임당 최초 1회만* 보고 (rate-limit)

### Cat F — 셰이더 컴파일·링크 실패 / warning
**실 사례**: lighting.fs 의 `texture2D` (deprecated) 사용 → ERROR cascade. `*fragColor` self-read.

**진단 조건**: 이미 `Program::CreateWithVSFS` 가 fail 시 nullptr 반환하지만, **info log** (warning 텍스트) 도 함께 캡처해 출력해야 함. 컴파일은 성공했지만 driver 가 warning 을 남기는 경우 검출.

---

## 3. API 표면 — 노출할 진단 함수

새 헤더: `src/diagnostics/gl_validate.h`

```cpp
namespace SJH::Diagnostics::GLValidate {
    /// Cat A: EBO 인덱스 OOB / degenerate triangle 검사.
    ///        CPU 측 std::vector 만으로 검사 (GL state 무관).
    /// @return 발견된 위반 개수 (0 = clean). 위반 내용은 spdlog::warn 로 출력.
    size_t CheckIndices(const std::vector<uint32_t>& indices, size_t vertexCount, const char* tag);

    /// Cat B: program 의 VS active attribute 와 현재 bound VAO 의 enabled attribute 비교.
    /// @pre 호출 직전 program 이 Use(), VAO 가 Bind() 된 상태여야 함.
    size_t CheckAttribLayout(GLuint program, const char* tag);

    /// Cat C: program 의 active uniform 목록 + CPU 호출 기록 비교.
    ///        CPU 측 호출 기록은 Uniforms:: 모듈에 hook 을 추가해 set 호출을 누적 (별도 PR 가능).
    ///        본 함수는 program 에 declared 이지만 default-0 인 uniform 만 보고 (보수적 진단).
    size_t CheckUniformCoverage(GLuint program, const char* tag);

    /// Cat D: program 의 sampler2D uniform 각각이 가리키는 texture unit 에 binding 이 있는지.
    size_t CheckSamplerBindings(GLuint program, const char* tag);

    /// Cat E: glGetError() 호출 + 마지막 보고한 코드 캐싱 (rate limit).
    ///        매 draw 후 호출 가능.
    bool CaptureGLError(const char* tag);

    /// Cat F: shader/program info log 가 비어있지 않으면 출력 (warning 텍스트 캡처).
    void DumpShaderInfoLogs(GLuint program, const char* tag);

    /// 통합 진단 — Init() 직후 한 번 호출하면 A·B·C·D·F 모두 실행.
    /// @return 0 = 모두 clean, > 0 = 위반 합계.
    size_t RunFullSweep(GLuint program,
                        const std::vector<uint32_t>& indices,
                        size_t vertexCount,
                        const char* tag);
}
```

---

## 4. 통합 지점 — 호출 위치

| 위치 | 호출 함수 | 호출 빈도 |
|---|---|---|
| `Mesh::Init()` 끝 | `CheckIndices(indices, vertices.size(), "mBox")` | 메시 생성 시 1회 |
| `Context::Init()` 끝 (`GLStateLog::Dump` 직후) | `RunFullSweep(mProgram->GetProgramAddr(), boxIndices, boxVertexCount, "lighting program")` | 1회 |
| `Context::Render()` 매 draw call 직후 (옵션) | `CaptureGLError("Render")` | 매 프레임 (rate-limited) |

---

## 5. 환경·도구

- **컴파일러**: clang (macOS) / MSVC (Windows). C++17.
- **빌드**: `cmake --build build_Darwin` (macOS), `cmake --build build_Windows --config Debug` (Windows)
- **로깅**: `#include <spdlog/spdlog.h>` 만 사용. fmt 는 spdlog 가 transitive 제공.
- **GL 호출**: `<glad/glad.h>` — 모든 `gl*` 함수는 glad 통해.
- **표준 라이브러리**: `<vector>`, `<string>`, `<unordered_set>` 정도. 외부 의존성 추가 금지.

---

## 6. 작업 단계 — 권장 순서

1. `src/diagnostics/gl_validate.h` / `gl_validate.cpp` 생성
2. `src/diagnostics/CMakeLists.txt` 에 `gl_validate.cpp` 추가 — 기존 `sjhopengl_diagnostics` 타깃 확장
3. Cat A → Cat E → Cat F → Cat B → Cat D → Cat C 순서로 구현 (단순한 것부터)
4. 각 카테고리마다 unit-level 자체 검증: 일부러 깨뜨린 mesh / shader 로 진단이 *false negative 없이* 잡아내는지 확인
5. `Context::Init()` / `Mesh::Init()` 에 통합 호출 추가
6. `cmake --build build_Darwin` 클린 통과 + 실행 시 spdlog 출력 정상 확인

---

## 7. Acceptance Criteria — 합격 기준

| # | 기준 | 검증 방법 |
|---|---|---|
| 1 | 빌드 클린 (에러 0, warning 추가 없음) | `cmake --build build_Darwin 2>&1 \| grep -iE "error\|warning:" \| grep -v "duplicate librar"` 결과 0줄 |
| 2 | 현재 정상 상태에서 `RunFullSweep` 이 0 위반 보고 | `./build_Darwin/app/OpenGL-With-CMake` 실행 후 로그에 `[GLValidate] all clean` 메시지 |
| 3 | 일부러 깨뜨린 케이스 6종 모두 탐지 | 아래 표 참조 |
| 4 | 매 프레임 호출 (`CaptureGLError`) 이 60FPS 유지 | rate limit 작동 — 로그 spam 없음 |
| 5 | macOS arm64 + Windows x64 양쪽에서 동작 | 플랫폼 분기 코드 없이 standard GL 함수만 사용 |

### Criterion 3 — 일부러 깨뜨리는 6 케이스

| Cat | 의도적 결함 | 기대 탐지 |
|---|---|---|
| A | mesh 인덱스를 `0, 0, 0` 으로 바꿈 | "degenerate triangle" warning |
| B | lighting.vs 에 `aColor` 다시 추가 | "VS expects loc 2 = vec3, VAO has vec2" warning |
| C | lighting.fs 에 `uniform vec3 unused;` 추가 | "declared but never set: unused" warning |
| D | `glActiveTexture(GL_TEXTURE3)` 호출 주석 처리 | "sampler material.specular → unit 3, but unit 3 unbound" warning |
| E | 임의 `glEnable(0xDEADBEEF)` 삽입 | `"GL_INVALID_ENUM after [tag]"` warning |
| F | lighting.fs 에 `varying float oldVarying;` (GL3 deprecated) | "shader info log: 'varying' deprecated..." warning |

---

## 8. 주의점 — 함정과 안티패턴

1. **`glGetError()` 호출 자체가 error state 를 clear 한다** — 진단 코드가 본 코드의 후속 에러 검사를 방해하지 않도록 *명시적* 호출 위치만. 본 코드에는 `glGetError` 사용 X.
2. **`glGet*` 류는 *동기화 stall*** — Init 1회 / Frame 끝 1회 외 호출 금지. 매 draw 후 `CaptureGLError` 만 예외 (가벼움).
3. **`glGetActiveAttrib / glGetActiveUniform` 의 `bufSize`** — 셰이더 변수 이름이 잘리지 않도록 `GL_ACTIVE_UNIFORM_MAX_LENGTH` / `GL_ACTIVE_ATTRIBUTE_MAX_LENGTH` 로 사전 조회.
4. **macOS Metal 호환 레이어** 가 `glGetIntegerv(GL_CURRENT_PROGRAM)` 등 일부 enum 에서 `GL_INVALID_OPERATION` 반환 — 진단이 *자기 자신* 의 호출로 false positive 일으키지 않도록 일부 `GL_*_BINDING` enum 은 try-and-clear 패턴 사용 (참고: `GLStateLog::Capture` 코드).
5. **CLAUDE.md §A.1** — 진단 코드는 RHI 추상화 예외 영역으로 간주, `gl*` 직접 호출 OK. PR 설명에 명시.

---

## 9. 산출물

- `src/diagnostics/gl_validate.h`
- `src/diagnostics/gl_validate.cpp`
- `src/diagnostics/CMakeLists.txt` (수정)
- `src/object/mesh.cpp` 1줄 (CheckIndices 호출)
- `src/context/context.cpp` 1줄 (RunFullSweep 호출)
- 짧은 진단 동작 데모 — README 형식 X, 단순 spdlog 출력 샘플 5~10줄을 PR 설명에 붙임

---

## 10. 본 작업에서 *생성하지 말 것*

- ❌ README.md 같은 문서 파일 (요청 없으면 작성 금지 — CLAUDE.md 일반 규칙)
- ❌ unit test framework 도입 (gtest 등) — 자체 검증은 일부러 깨뜨린 케이스 수동 실행으로 충분
- ❌ CI workflow / GitHub Actions 추가
- ❌ Mesa llvmpipe / FLIP 통합 (별도 Phase)

---

## 11. 완료 보고 형식

작업 완료 후 다음 형식으로 보고:

1. **Acceptance Criteria 표** (위 §7) 각 행에 ✅ / ❌
2. **spdlog 샘플 출력 5~10줄** — 정상 상태 + 깨뜨린 케이스 각각
3. **수정한 파일 목록** (각 1줄 요약)
4. **남은 이슈 / 추후 작업** (있다면 3개 이내)

그 외 작업 진행 사항 narration 은 최소화.
