# https://csj000714.tistory.com/414
# https://github.com/cpichard/usdtweak/blob/develop/CMakeLists.txt
# 최소 CMkae버젼 요구사항 명시
cmake_minimum_required(VERSION 3.14.0)

set(CMAKE_CXX_STANDARD 17)
set(PROJECT_NAME cmake_project-example)
# 프로젝트 이름 지정
project(${PROJECT_NAME})


# 외부 라이브러리를 다운로드하고 빌드에 포함하기 위한 모듈입니다.
# 프로젝트의 일환으로 외부 의존성을 직접 가져와 사용하고 싶을 때 유용합니다.

include(FetchContent)

FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
)

FetchContent_MakeAvailable(spdlog)

# glfw
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
)

FetchContent_MakeAvailable(glfw)

# linmath
FetchContent_Declare(linmath
    GIT_REPOSITORY https://github.com/datenwolf/linmath.h.git
)

## ??
FetchContent_MakeAvailable(linmath)

# glad
FetchContent_Declare(glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
)

## ??
FetchContent_MakeAvailable(glad)

# 타겟 프로그램 정의
add_executable(${PROJECT_NAME} src/main.cpp)

## ??
target_link_libraries(${PROJECT_NAME}
PRIVATE
    spdlog
    glfw
    linmath
    glad
)