# 타겟을 링킹할 때 필요한 타겟이나 라이브러러리를 지정
# 라이브러리 지정시 ::은 namespace까지도 의미하는것으로 보임?

# add_executable(${PROJECT_NAME}
#     Common/common.cpp                   Common/common.h
#     Shader/shader.cpp                   Shader/shader.h
#     Program/program.cpp                 Program/program.h
#     Context/context.cpp                 Context/context.h
#     Buffer/buffer.cpp                   Buffer/buffer.h
#     VertexLayout/vertex_layout.cpp      VertexLayout/vertex_layout.h
#     Image/image.cpp                     Image/image.h
# )

target_include_directories(${PROJECT_NAME}
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Common
    ${CMAKE_CURRENT_SOURCE_DIR}/Shader
    ${CMAKE_CURRENT_SOURCE_DIR}/Program
    ${CMAKE_CURRENT_SOURCE_DIR}/Context
    ${CMAKE_CURRENT_SOURCE_DIR}/Buffer
    ${CMAKE_CURRENT_SOURCE_DIR}/VertexLayout
    ${CMAKE_CURRENT_SOURCE_DIR}/Image
)
