# https://csj000714.tistory.com/414
# https://github.com/cpichard/usdtweak/blob/develop/CMakeLists.txt
# 최소 CMkae버젼 요구사항 명시

# 필요한 CMake 최소 버전 명시
cmake_minimum_required(VERSION 3.11.0)

# 사용할 C++ 버전을 20으로 설정
set(PROJECT_NAME cmake_project-example)
set(CMAKE_CXX_STANDARD 20)

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

# 외부 라이브러리를 다운로드하고 빌드에 포함하기 위한 FetchContent 모듈 로드
include(FetchContent)

FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG 706ad7059125f32158dad4441938c08fa910f143
)

FetchContent_MakeAvailable(spdlog)

# 타겟 프로그램 정의
add_executable(${PROJECT_NAME} src/main.cpp)

# 타겟을 링킹할 때 필요한 타겟이나 라이브러러리를 지정
target_link_libraries(${PROJECT_NAME} 
PRIVATE
    spdlog::spdlog
)