add_library(sjhopengl_resource_management STATIC
    resource_management.cpp
)
add_library(SJH::resource_management ALIAS sjhopengl_resource_management)

target_include_directories(sjhopengl_resource_management
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(sjhopengl_resource_management PRIVATE cxx_std_17)
