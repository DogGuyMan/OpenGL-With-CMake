# https://csj000714.tistory.com/414
# https://github.com/cpichard/usdtweak/blob/develop/CMakeLists.txt
# https://jonathanhamberg.com/post/2019-01-27-cmake-dependencies-fetch-content/
# 최소 CMkae버젼 요구사항 명시

# 필요한 CMake 최소 버전 명시
cmake_minimum_required(VERSION 3.13)

# 사용할 C++ 버전을 20으로 설정
set(PROJECT_NAME opengl_example)
set(CMAKE_CXX_STANDARD 17)

# 윈도우 해상도 설정하는 변수 설정
set(WINDOW_NAME "OpenGL Example")
set(WINDOW_WIDTH 800)
set(WINDOW_HEIGHT 600)

## cocoa framework

# ```
# if (APPLE)
#     set(CMAKE_CXX_FLAGS "-framework Cocoa -framework IOKit -framework OpenGL")
# endif()
# ```

# 프로젝트 이름 지정
project(${PROJECT_NAME})

# 모듈 로드
include(CheckIncludeFileCXX)

# C++ 파일이 있는지 검사
check_include_file_cxx("filesystem" CXX_FILESYSTEM_HAVE_HEADER)

# 파일이 없는 경우 메세지 출력
if(NOT CXX_FILESYSTEM_HAVE_HEADER)
    message(STATUS "Oh no! can't use filesystem!")
endif()

# Dpendency.cmake 파일을 include
include(Dependency.cmake)

# 타겟 실행파일 정의
add_executable(${PROJECT_NAME} 
    src/main.cpp
    include/common.cpp include/common.h
    include/shader.cpp include/shader.h
)

# 타겟을 링킹할 때 필요한 타겟이나 라이브러러리를 지정
# 라이브러리 지정시 ::은 namespace까지도 의미하는것으로 보임?
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEP_LIBS})

target_include_directories(${PROJECT_NAME}
PUBLIC
    include
PRIVATE 
    src
)

# 컴파일러에게 전달할 매크로 #define 설정
target_compile_definitions(${PROJECT_NAME}
PUBLIC
    WINDOW_NAME="${WINDOW_NAME}"
    WINDOW_WIDTH=${WINDOW_WIDTH}
    WINDOW_HEIGHT=${WINDOW_HEIGHT}
)