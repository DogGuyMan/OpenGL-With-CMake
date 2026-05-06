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
if(SJH_OPENGL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

#  Documentation
if(SJH_OPENGL_BUILD_DOCS)
    sjhopengl_setup_doxygen()
endif()
