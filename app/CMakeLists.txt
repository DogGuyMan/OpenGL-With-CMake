# 타겟 실행파일 정의
add_executable(${PROJECT_NAME}
    main.cpp
    ${PROJECT_SOURCE_DIR}/src/Common/common.cpp
    ${PROJECT_SOURCE_DIR}/src/Shader/shader.cpp
    ${PROJECT_SOURCE_DIR}/src/Program/program.cpp
    ${PROJECT_SOURCE_DIR}/src/Context/context.cpp
    ${PROJECT_SOURCE_DIR}/src/Buffer/buffer.cpp
    ${PROJECT_SOURCE_DIR}/src/VertexLayout/vertex_layout.cpp
    ${PROJECT_SOURCE_DIR}/src/Image/image.cpp
)
