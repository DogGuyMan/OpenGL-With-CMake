# OpenGL With CMake

> ### CLI로 CMake GNU 파일 제작 Shell 스크립트 실행하기


<div align=center>
    <img src="image/2024-10-11-16-48-35.png" width=50%>
</div>

```bash
# sh GNU_DirectoryStructure.sh <ProjectName>

# 본인의 경우 다음과 같이 생성함
sh GNU_DirectoryStructure.sh CMAKE_PROJECT_EXAMPLE
```

---

> ### CMake

#### 0). 빌드와 실행법
```
cmake -B build
cmake --build build

./build/opengl_example
```

#### 1). FecthContent시 주의사항
```
glad를 다운받는 예시

CMakeLists의 Dependency.cmake를 에서 

FetchContent_Declare 에서 
target을 "glad" 라고 하는 대신 
target을 "dep-gald" 로 하여 의존성 추가하니 작동됨
```

```txt
// FetchContent_Declare(glad
//     GIT_REPOSITORY https://github.com/Dav1dde/glad.git
//     GIT_TAG "v0.1.34"
//     GIT_SHALLOW 1
// )
// 
// set(DEP_LIST ${DEP_LIST} dep-glad)
// set(DEP_LIBS ${DEP_LIBS} glad)

FetchContent_Declare(dep-glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG "v0.1.34"
    GIT_SHALLOW 1
)

set(DEP_LIST ${DEP_LIST} dep-glad)
set(DEP_LIBS ${DEP_LIBS} glad)
```

> ### 참고 강의

#### 1). [삼각형의 실전! CMake 초급](https://www.inflearn.com/course/%EC%8B%A4%EC%A0%84-cmake-%EC%B4%88%EA%B8%89/dashboard) 
* 이 강의를 통해 ...
    1. CMake CLI, 
    2. C/C++ 라이브러리 의존성 관리
    3. 모던 CMake의 모듈러 디자인에 대해 이해했고, 확장성 있는 빌드 시스템 작성법을 배웠음

#### 2). [Rinthel Kwon OpenGL course](https://www.youtube.com/watch?v=kEAKvJKnvfA&list=PLvNHCGtd4kh_cYLKMP_E-jwF3YKpDP4hf&ab_channel=RinthelKwon)

* 이 강의를 통해
  1. 삼각형의 CMake 빌드시스템 작성법과 상호 대조해 보며 개발 환경 세팅을 진행했고,
  2. 그래픽스 이론을 학습하기 
