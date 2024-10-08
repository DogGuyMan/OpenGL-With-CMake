# 최소 CMkae버젼 요구사항 명시
cmake_minimum_required(VERSION 3.14.0)

set(CMAKE_CXX_STANDARD 17)
set(PROJECT_NAME cmake_project-example)
# 프로젝트 이름 지정
project(${PROJECT_NAME})

# 타겟 프로그램 정의
add_executable(${PROJECT_NAME} src/main.cpp)
