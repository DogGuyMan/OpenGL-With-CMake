# 외부 라이브러리를 다운로드하고 빌드에 포함하기 위한 FetchContent 모듈 로드
include(FetchContent)

# ./build/_deps 에 추가되고, 해당 디렉토리의 CMakeLists.txt도 서브 디렉토리에 추가됨.
# dep의 유래는... dependency의 줄임말인가?


# Dependency 관련 변수 설정

# spdlog: fast logger library
FetchContent_Declare(dep-spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG "v1.x"
    GIT_SHALLOW 1 # 1==true 로 하면 깃 변경 내역 최신것만 받아온다는 의미.
)
# Dependency 리스트 및 라이브러리 파일 리스트 추가
set(DEP_LIST ${DEP_LIST} dep-spdlog)
set(DEP_LIBS ${DEP_LIBS} spdlog::spdlog)

# glfw
FetchContent_Declare(dep-glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG "3.3.2"
    GIT_SHALLOW 1
)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(DEP_LIST ${DEP_LIST} dep-glfw)
set(DEP_LIBS ${DEP_LIBS} glfw)

# glad
FetchContent_Declare(dep-glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG "v0.1.34"
    GIT_SHALLOW 1
)

set(DEP_LIST ${DEP_LIST} dep-glad)
set(DEP_LIBS ${DEP_LIBS} glad)

FetchContent_MakeAvailable(${DEP_LIST})