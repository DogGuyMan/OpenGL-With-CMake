add_library(sjhopengl_program STATIC
    program.cpp
    program_uniforms.cpp
)
add_library(SJH::program ALIAS sjhopengl_program)

target_include_directories(sjhopengl_program
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

#include "diagnostics/gl_log.h"
#include "common/common.h"
#include "shader/shader.h"
target_link_libraries(sjhopengl_program
    PUBLIC  SJH::common
            SJH::shader
            glad::glad
    PRIVATE SJH::diagnostics
)

target_compile_features(sjhopengl_program PRIVATE cxx_std_17)
