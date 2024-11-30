# 외부 라이브러리를 다운로드하고 빌드에 포함하기 위한 FetchContent 모듈 로드
include(FetchContent)

# ./build/_deps 에 추가되고, 해당 디렉토리의 CMakeLists.txt도 서브 디렉토리에 추가됨.
# dep의 유래는... dependency의 줄임말인가?


# Dependency 관련 변수 설정

# spdlog: fast logger library
FetchContent_Declare(dep_spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG "v1.x"
    GIT_SHALLOW 1 # 1==true 로 하면 깃 변경 내역 최신것만 받아온다는 의미.
)
# Dependency 리스트 및 라이브러리 파일 리스트 추가
set(DEP_LIST ${DEP_LIST} dep_spdlog)
set(DEP_LIBS ${DEP_LIBS} spdlog::spdlog)

# glfw
FetchContent_Declare(dep_glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG "3.3.2"
    GIT_SHALLOW 1
    CMAKE_ARGS 
        -DGLFW_BUILD_EXAMPLES=OFF
        -DGLFW_BUILD_TESTS=OFF
        -DGLFW_BUILD_DOCS=OFF
)

set(DEP_LIST ${DEP_LIST} dep_glfw)
set(DEP_LIBS ${DEP_LIBS} glfw)

# glad
FetchContent_Declare(dep_glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG "v0.1.34"
    CMAKE_ARGS
        -DGLAD_INSTALL=ON
)

set(DEP_LIST ${DEP_LIST} dep_glad)
set(DEP_LIBS ${DEP_LIBS} glad)

# stb
FetchContent_Declare(dep_stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG "master"
    GIT_SHALLOW 1
)
set(DEP_LIST ${DEP_LIST} dep_stb)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_BINARY_DIR}/_deps/dep_stb-src/stb_image.h
    ${CMAKE_BINARY_DIR}/_deps/dep_stb-build/stb_image.h
)

FetchContent_MakeAvailable(${DEP_LIST})