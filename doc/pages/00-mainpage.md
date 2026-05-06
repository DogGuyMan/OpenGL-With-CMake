# OpenGL-With-CMake {#mainpage}

OpenGL 학습 프로젝트 — C++17 + CMake + vcpkg manifest mode.

## 모듈 레이어 다이어그램

\dot
digraph LayerArchitecture {
  rankdir=BT;
  node [shape=box, style=rounded, fontname="Helvetica"];

  subgraph cluster_app {
    label="앱 (라이프 사이클)";
    style=dashed;
    main [label="app/main.cpp"];
  }

  subgraph cluster_scene {
    label="씬 캡슐화 (Context)";
    style=dashed;
    Context;
  }

  subgraph cluster_modules {
    label="GL 모듈";
    style=dashed;
    Program; Shader;
  }

  subgraph cluster_base {
    label="기반";
    style=dashed;
    Common; Diagnostics;
  }

  subgraph cluster_external {
    label="외부 (vcpkg)";
    style=filled; fillcolor="#f0f0f0";
    glfw; glad; spdlog; fmt;
  }

  main -> Context;
  Context -> Program;
  Context -> Shader;
  Program -> Shader;
  Shader -> Common;
  Shader -> Diagnostics;
  Program -> Diagnostics;
  Context -> Diagnostics;
  Common -> spdlog;
  Diagnostics -> spdlog;
  Diagnostics -> glad;
  Shader -> glad;
  Program -> glad;
  Context -> glad;
  main -> glfw;
}
\enddot

## 렌더링 시퀀스 (high-level)

\dot
digraph RenderSequence {
  rankdir=LR;
  node [shape=box, fontname="Helvetica"];
  Start [label="main()", shape=ellipse];
  init [label="GLFW init\n+ glad load"];
  ctx [label="Context::Create()\n→ shaders → program → VAO"];
  loop [label="while (!shouldClose)\n  Context::Render()\n  swapBuffers", shape=box, style="rounded,filled", fillcolor="#fff7d6"];
  term [label="GLFW terminate", shape=ellipse];
  Start -> init -> ctx -> loop -> term;
}
\enddot

## 추가 페이지

- @ref build-system "빌드 시스템 가이드"
- @ref dependencies "의존 라이브러리"

## 핵심 모듈

| 모듈 | 클래스 | 역할 |
|------|--------|------|
| common | — | @c CLASS_PTR 매크로 + @ref SJH::LoadTextFile |
| diagnostics | @ref SJH::Diagnostics::GLObjectLog "GLObjectLog", @ref SJH::Diagnostics::GLDebug "GLDebug" | GL 에러 로깅 일원화 |
| shader | @ref SJH::Shader "Shader" | 셰이더 컴파일 RAII |
| program | @ref SJH::Program "Program" | 프로그램 링크 RAII |
| context | @ref SJH::Context "Context" | 씬 자원 + 매 프레임 draw |
