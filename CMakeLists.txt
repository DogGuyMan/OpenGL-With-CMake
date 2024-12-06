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

# 타겟 실행파일 정의
add_subdirectory(app)
add_subdirectory(src)

include(Dependency)

# 타겟을 링킹할 때 필요한 타겟이나 라이브러러리를 지정
# 라이브러리 지정시 ::은 namespace까지도 의미하는것으로 보임?

target_link_libraries(${PROJECT_NAME} PUBLIC ${DEP_LIBS})
target_include_directories(${PROJECT_NAME} PRIVATE ${DEP_INCLUDE_DIR})

foreach(dep IN LISTS ${DEP_LIST})
execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${${dep}_BINARY_DIR}
    COMMAND echo "Building ${dep} on ${${dep}_BINARY_DIR}"
)
endforeach()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/_deps/dep_stb-src/stb_image.h ${CMAKE_BINARY_DIR}/install/include/stb/stb_image.h
)


# 컴파일러에게 전달할 매크로 #define 설정
target_compile_definitions(${PROJECT_NAME}
PUBLIC
    WINDOW_NAME="${WINDOW_NAME}"
    WINDOW_WIDTH=${WINDOW_WIDTH}
    WINDOW_HEIGHT=${WINDOW_HEIGHT}
)
