add_library(sjhopengl_object INTERFACE)

add_library(SJH::object ALIAS sjhopengl_object)

target_include_directories(sjhopengl_object
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)
target_link_libraries(sjhopengl_object
    INTERFACE glm::glm                 # camera.h 가 glm::vec3 등 사용한다면
)
target_compile_features(sjhopengl_object INTERFACE cxx_std_17)
