# 의존 라이브러리 {#dependencies}

본 프로젝트는 vcpkg manifest mode([vcpkg.json](../../vcpkg.json))로 의존성을 관리한다.
`cmake --preset <name>` 시점에 자동 설치되며, `cmake/Dependency.cmake` 가 `find_package` 호출.

## 패키지 표

| 패키지 | vcpkg 명 | CMake 타겟 | 사용처 |
|--------|----------|-----------|--------|
| **fmt** | `fmt` | `fmt::fmt` | 포매팅 (spdlog 백엔드 + 직접 사용) |
| **spdlog** | `spdlog` | `spdlog::spdlog` | 모든 모듈의 로깅 백엔드 |
| **GLFW** | `glfw3` | `glfw` (네임스페이스 없음) | 윈도우/입력/GL 컨텍스트 — `app/main.cpp` |
| **glad** | `glad` | `glad::glad` | OpenGL 함수 로더 — 모든 GL 모듈 |
| **stb** | `stb` | `${Stb_INCLUDE_DIR}` (헤더 only) | 이미지 로딩 (텍스처) — `app/` |
| **Catch2** | `catch2` | `Catch2::Catch2WithMain` | 단위 테스트 (`test/`) |

## 의존성 그래프 (모듈 → 외부)

\dot
digraph DepGraph {
  rankdir=LR;
  node [shape=box, fontname="Helvetica"];

  subgraph cluster_internal {
    label="내부 모듈";
    style=dashed;
    common; diagnostics; shader; program; context;
  }

  subgraph cluster_external {
    label="외부 (vcpkg)";
    style=filled; fillcolor="#f0f0f0";
    fmt; spdlog; glfw; glad; stb; catch2;
  }

  common -> spdlog;
  diagnostics -> spdlog;
  diagnostics -> fmt;
  diagnostics -> glad;
  shader -> glad;
  shader -> common;
  shader -> diagnostics;
  program -> glad;
  program -> shader;
  program -> diagnostics;
  context -> program;
  context -> shader;
  context -> diagnostics;
  context -> glad;
}
\enddot

## 명시적 의존성 설치 타겟

```bash
cmake --build build_Darwin --target install-deps
```

내부적으로 `$ENV{VCPKG_ROOT}/vcpkg install --triplet ${VCPKG_TARGET_TRIPLET}` 호출.
`vcpkg.json` 변경 후 재구성 없이 의존성만 갱신할 때 사용.

## 트러블슈팅

| 증상 | 원인 | 해결 |
|------|------|------|
| `ld: library 'fmt' not found` | `fmt::fmt` 가 아닌 `fmt` 로 링크 | `target_link_libraries(... fmt::fmt)` |
| `ninja required v1.13.2` | 구버전 ninja | `brew install ninja` |
| glad "OpenGL header already included" | glfw3.h가 glad보다 먼저 include | `glad/glad.h` 를 가장 위에 |

자세한 내용은 `.claude/build-system.md` §의존성 트러블슈팅.
