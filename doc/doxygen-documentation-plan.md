# Doxygen 문서화 계획

> 본 문서는 `newEnv` 브랜치 src/ 모듈에 Doxygen 주석을 단계적으로 추가하고, 빌드 시스템·다이어그램을 포함한 통합 문서를 산출하는 계획서다.
>
> **목표**: `cmake --build build_Darwin --target doxygen` 실행 시 모듈 API + 클래스 다이어그램 + 의존성 그래프 + 빌드 시스템 가이드를 포함한 HTML 문서가 `doc/html/` 에 생성되어야 한다.

## 1. 산출물

| 산출물 | 위치 | 생성 방식 |
|--------|------|----------|
| 모듈 API 레퍼런스 | `doc/html/` | Doxygen 자동 (헤더 주석 → HTML) |
| 클래스 상속/포함 그래프 | `doc/html/` 내 임베드 | Doxygen + Graphviz 자동 |
| 헤더 include 그래프 | `doc/html/` 내 임베드 | Doxygen + Graphviz 자동 |
| 모듈 책임/레이어 다이어그램 | `\page` 메인 페이지 | PlantUML 수동 (의도 표현) |
| 빌드 시스템 가이드 | `\page` | `.claude/build-system.md` 변환 또는 링크 |
| 의존 라이브러리 표 | `\page` | vcpkg.json 패키지 + 사용처 매핑 |

## 2. 주석 스타일 합의

- **언어**: 한국어 (기존 `diagnostics/gl_log.h` 톤 유지)
- **범위**: 헤더(.h)에만 표준 Doxygen 주석. .cpp는 비-자명한 의도만 한 줄 인라인.
- **태그 사용**: `@brief`, `@param`, `@return`, `@details`, `@note`, `@warning`, `@code` (필요 시)
- **레퍼런스 모범 사례**: [src/diagnostics/gl_log.h](../src/diagnostics/gl_log.h)

## 3. Phase 구성 (의존성 leaf → root)

각 Phase 진행 사이클:
1. 해당 모듈 헤더 현재 상태 확인
2. 추가/변환할 Doxygen draft 제시
3. 사용자 검토 → 톤·내용 OK 시 적용
4. 다음 Phase

| Phase | 모듈 | 의존 | 비고 |
|-------|------|------|------|
| **1** | `common` + `diagnostics` 검토 | (없음) | `CLASS_PTR` 매크로 톤 합의가 핵심. diagnostics는 이미 완성, 검토만. |
| **2** | `shader` | common, diagnostics | 팩토리 패턴(`CreateFromFile`) 한글 블록 코멘트 → Doxygen 변환 |
| **3** | `program` | shader | shader와 유사 (벡터 입력 차이) |
| **4** | `context` | program | "Context 책임 분담" 설계 철학 보존 + 변환 |
| **5** | placeholder 모듈 | — | `buffer` / `layout` / `resource_management` 짧은 placeholder 주석 |
| **6** | 다이어그램 + 페이지 | — | Doxyfile 갱신 (HAVE_DOT 등) + `\page` 메인 + PlantUML |

## 4. 비활성 모듈 처리 정책

- `buffer`, `layout`, `resource_management` 는 `src/CMakeLists.txt` 에서 `add_subdirectory` 주석 처리됨
- Doxygen `EXTRACT_ALL=YES` 이므로 빈 namespace도 출력 → 짧은 placeholder 주석 추가하여 "의도된 빈 상태" 명시
- 향후 마이그레이션 시 [migration-plan.md](migration-plan.md) 참조 한 줄 포함

## 5. 다이어그램 전략 (A + B 병행)

### A. Doxygen + Graphviz 자동 그래프 (사실)
- Doxyfile 변경: `HAVE_DOT=YES`, `CLASS_GRAPH=YES`, `COLLABORATION_GRAPH=YES`, `INCLUDE_GRAPH=YES`, `INCLUDED_BY_GRAPH=YES`, `CALL_GRAPH=NO`(노이즈), `DOT_IMAGE_FORMAT=svg`
- 의존성: `brew install graphviz` (macOS), Windows는 vcpkg 또는 별도 설치
- 코드와 항상 동기화 — 클래스 상속, include 관계

### B. PlantUML 수동 다이어그램 (설계 의도)
- Doxygen `\dot ... \enddot` 또는 `\plantuml` 디렉티브 사용
- 대상:
  - **레이어 다이어그램**: `app` → `context` → (`program`, `shader`) → (`common`, `diagnostics`) → 외부(`glad`, `glfw`, `spdlog`)
  - **렌더링 시퀀스**: `main` → `Context::Create` → `Context::Render` 흐름
  - **팩토리/RAII 패턴 설명도**: `Shader::CreateFromFile` 의 nullptr 반환 시 자원 정리 흐름

### 통합
- 메인 페이지(`\page index`)에 PlantUML 임베드 + 자동 그래프 링크
- README.md를 `USE_MDFILE_AS_MAINPAGE=README.md` 로 메인 페이지로 사용 (Doxyfile 기존 설정 유지)

## 6. Doxyfile 변경 항목 (Phase 6 작업)

| 키 | 현재값 | 목표값 | 이유 |
|----|--------|--------|------|
| `HAVE_DOT` | (미설정) | `YES` | Graphviz 활성화 |
| `DOT_IMAGE_FORMAT` | (기본 png) | `svg` | 확대해도 선명 |
| `CLASS_GRAPH` | (기본 YES) | `YES` | 명시 |
| `COLLABORATION_GRAPH` | (기본 NO) | `YES` | 멤버 객체 관계 시각화 |
| `INCLUDE_GRAPH` | (기본 YES) | `YES` | 명시 |
| `INCLUDED_BY_GRAPH` | (기본 YES) | `YES` | 명시 |
| `CALL_GRAPH` | (기본 NO) | `NO` | 노이즈 회피 |
| `EXTRACT_PRIVATE` | `NO` | `NO` 유지 | 캡슐화 정보 차단 |
| `WARN_IF_UNDOCUMENTED` | `YES` | `YES` 유지 | 누락 검출 |
| `INPUT` | `@DOXYGEN_INPUT_DIR@` | `@DOXYGEN_INPUT_DIR@ @DOXYGEN_PAGES_DIR@` | `\page` 마크다운 포함 |

## 7. 검증 방법

각 Phase 완료 시:
```bash
cmake --build build_Darwin --target doxygen
open doc/html/index.html  # macOS
```

확인 항목:
- 새 주석이 HTML에 반영되었는가?
- `WARN_IF_UNDOCUMENTED` 경고가 줄어들었는가?
- (Phase 6 이후) 다이어그램이 SVG로 렌더되는가?

## 8. 산출물 위치 정리

```
doc/
├── Doxyfile.in                    Phase 6 에서 갱신
├── doxygen-documentation-plan.md  본 문서
├── pages/                         Phase 6 신설 — \page 마크다운 모음
│   ├── 00-mainpage.md             메인 + 레이어 다이어그램
│   ├── 10-build-system.md         빌드 가이드
│   └── 20-dependencies.md         vcpkg 의존성
└── html/                          Doxygen 생성 출력 (gitignore)
```

## 9. 범위 외 (out of scope)

- src/ 외부 코드(`app/main.cpp`, `include/input/`, `test/`) Doxygen 주석 — 이번 작업 제외
- 영문 번역 — 한국어 단일
- README.md 갱신 — 별도 작업
- CI 통합 — 별도 작업
