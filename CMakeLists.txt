# https://csj000714.tistory.com/414
# https://github.com/cpichard/usdtweak/blob/develop/CMakeLists.txt
# https://jonathanhamberg.com/post/2019-01-27-cmake-dependencies-fetch-content/
# 최소 CMake버젼 요구사항 명시

# 필요한 CMake 최소 버전 명시
cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME OpenGLProject)
project(${PROJECT_NAME})

# 사용할 C++ 버전을 20으로 설정
set(CMAKE_CXX_STANDARD 17)

# 윈도우 해상도 설정하는 변수 설정
set(WINDOW_NAME "OpenGL Example")
set(WINDOW_WIDTH 800)
set(WINDOW_HEIGHT 600)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

# 프로젝트 이름 지정

# 모듈 로드
include(Dependency)

# Executable에 사용하는 Source들과 Header 모음
set(PROJECT_SOURCES
    src/buffer.cpp src/common.cpp src/context.cpp src/image.cpp src/program.cpp
    src/shader.cpp src/vertex_layout.cpp src/texture.cpp
)

set(PROJECT_HEADERS
    src/buffer.h src/common.h src/context.h src/image.h src/program.h
    src/shader.h src/vertex_layout.h src/texture.h
)

# Executable 설정
add_executable(${PROJECT_NAME}
    app/main.cpp
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

# 타겟을 링킹할 때 필요한 헤더 지정
# src도 포함해야한다!
target_include_directories(${PROJECT_NAME} PRIVATE
    ${DEP_INCLUDE_DIR}
    src
)
# 타겟을 링킹할 때 필요한 라이브러리 지정
target_link_libraries(${PROJECT_NAME} PUBLIC ${DEP_LIBS})

# 컴파일러에게 전달할 매크로 #define 설정
target_compile_definitions(${PROJECT_NAME}
PUBLIC
    WINDOW_NAME="${WINDOW_NAME}"
    WINDOW_WIDTH=${WINDOW_WIDTH}
    WINDOW_HEIGHT=${WINDOW_HEIGHT}
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/_deps/dep_stb-src/stb_image.h ${CMAKE_BINARY_DIR}/install/include/stb/stb_image.h
)
