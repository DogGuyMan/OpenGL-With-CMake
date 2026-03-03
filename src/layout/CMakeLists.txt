add_library(sjhopengl_layout STATIC
    vertex_layout.cpp
)
add_library(SJH::layout ALIAS sjhopengl_layout)

target_include_directories(sjhopengl_layout
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(sjhopengl_layout PRIVATE cxx_std_17)
