# 타겟을 링킹할 때 필요한 타겟이나 라이브러러리를 지정
# 라이브러리 지정시 ::은 namespace까지도 의미하는것으로 보임?
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/shader
    ${CMAKE_CURRENT_SOURCE_DIR}/program
    ${CMAKE_CURRENT_SOURCE_DIR}/context
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer
    ${CMAKE_CURRENT_SOURCE_DIR}/vertex_layout
    ${CMAKE_CURRENT_SOURCE_DIR}/image
    ${CMAKE_BINARY_DIR}/install/include
)
