# cmake --preset debug -DVCPKG_MANIFEST_INSTALL=OFF

cmake_minimum_required(VERSION 3.21)
set(PROJECT_NAME "OpenGL-With-CMake")
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES CXX)

#  Export compile_commands.json for IDE / clangd

#  Options
option(SJH_OPENGL_BUILD_TESTS "Build test executables" ON)
option(SJH_OPENGL_BUILD_DOCS  "Enable Doxygen doc target" ON)

#  CMake helper modules
include(cmake/CXXStandard.cmake)
include(cmake/Doxygen.cmake)
include(cmake/Dependency.cmake)
include(cmake/Config.cmake)

#  External dependencies (vcpkg manifest mode)
# find_package(SDL2 CONFIG REQUIRED)

#  Source modules (static libraries)
add_subdirectory(src)

#  Main executable
add_subdirectory(app)

#  Tests
# EXCLUDE_FROM_ALL: test 타겟은 default `cmake --build` (=ALL)에서 제외.
# test_* 컴파일 에러가 app+src 빌드를 실패시키지 않도록.
# 명시 빌드: `cmake --build <dir> --target tests` (umbrella) 또는 `--target test_buffer` 등.
if(SJH_OPENGL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test EXCLUDE_FROM_ALL)
endif()

#  Documentation
if(SJH_OPENGL_BUILD_DOCS)
    sjhopengl_setup_doxygen()
endif()
