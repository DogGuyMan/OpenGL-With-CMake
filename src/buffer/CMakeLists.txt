add_library(sjhopengl_buffer STATIC
    buffer.cpp
)
add_library(SJH::buffer ALIAS sjhopengl_buffer)

target_include_directories(sjhopengl_buffer
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(sjhopengl_buffer PRIVATE cxx_std_17)
