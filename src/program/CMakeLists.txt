add_library(sjhopengl_program STATIC
    program.cpp
)
add_library(SJH::program ALIAS sjhopengl_program)

target_include_directories(sjhopengl_program
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(sjhopengl_program PRIVATE cxx_std_17)
