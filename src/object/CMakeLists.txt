add_library(sjhopengl_object STATIC mesh.cpp model.cpp)
add_library(SJH::object ALIAS sjhopengl_object)

target_include_directories(sjhopengl_object
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sjhopengl_object
    PUBLIC  SJH::common      # mesh.h: CLASS_PTR(Mesh) 매크로
            SJH::buffer      # mesh.h: Buffer / BufferPtr 필드
            SJH::layout      # mesh.h: VertexLayout / VertexLayoutUPtr 필드
            glad::glad       # mesh.h: <glad/glad.h>, GLuint
            glm::glm         # mesh.h / camera.h / light.h: glm::vec3 등
            assimp::assimp
)

target_compile_features(sjhopengl_object PUBLIC cxx_std_17)
