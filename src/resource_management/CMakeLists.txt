add_library(sjhopengl_resource_management STATIC
    resource_management.cpp
    texture.cpp
    image.cpp
)
add_library(SJH::resource_management ALIAS sjhopengl_resource_management)

target_include_directories(sjhopengl_resource_management
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${Stb_INCLUDE_DIR}
)

target_link_libraries(sjhopengl_resource_management
    PUBLIC  SJH::common
            glad::glad
            glm::glm
            spdlog::spdlog
    PRIVATE SJH::diagnostics
)

target_compile_features(sjhopengl_resource_management PRIVATE cxx_std_17)
