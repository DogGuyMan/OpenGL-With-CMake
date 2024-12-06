# https://csj000714.tistory.com/414
# https://github.com/cpichard/usdtweak/blob/develop/CMakeLists.txt
# https://jonathanhamberg.com/post/2019-01-27-cmake-dependencies-fetch-content/
# 최소 CMake버젼 요구사항 명시

# 필요한 CMake 최소 버전 명시
cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME opengl_example)
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
include(CheckIncludeFileCXX)

# C++ 파일이 있는지 검사
check_include_file_cxx("filesystem" CXX_FILESYSTEM_HAVE_HEADER)

# 파일이 없는 경우 메세지 출력
if(NOT CXX_FILESYSTEM_HAVE_HEADER)
    message(STATUS "Oh no! can't use filesystem!")
endif()


# 타겟 실행파일 정의
add_subdirectory(app)
add_subdirectory(src)

# 컴파일러에게 전달할 매크로 #define 설정
target_compile_definitions(${PROJECT_NAME}
PUBLIC
    WINDOW_NAME="${WINDOW_NAME}"
    WINDOW_WIDTH=${WINDOW_WIDTH}
    WINDOW_HEIGHT=${WINDOW_HEIGHT}
)
