add_library(sjhopengl_context STATIC
    context.cpp
)
add_library(SJH::context ALIAS sjhopengl_context)

target_include_directories(sjhopengl_context
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(sjhopengl_context PRIVATE cxx_std_17)
