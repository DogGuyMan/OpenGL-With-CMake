add_library(sjhopengl_diagnostics STATIC
    gl_log.cpp
    uniform_diagnostics.cpp
    gl_state_fields.cpp
    gl_state_log.cpp
    gl_validate.cpp
)
add_library(SJH::diagnostics ALIAS sjhopengl_diagnostics)

target_include_directories(sjhopengl_diagnostics
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# glad 는 헤더(gl_log.h)에서 GLuint/GLenum 사용 -> PUBLIC
# spdlog 는 비-GL 일반 로깅에도 자유롭게 쓰이도록 PUBLIC 전파
# (이 모듈의 책임은 GL 진단 패턴 중앙화이지 spdlog 은폐가 아님)
target_link_libraries(sjhopengl_diagnostics
    PUBLIC  glad::glad
            spdlog::spdlog
    PRIVATE fmt::fmt
)

target_compile_features(sjhopengl_diagnostics PRIVATE cxx_std_17)
