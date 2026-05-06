#  Tests
# 각 test_*.cpp 는 독립 실행 파일 (Catch2WithMain 링크).
# 자세한 설계 의도는 doc/testing-curriculum.md 참조.

find_package(Catch2 3 CONFIG REQUIRED)

include(CTest)
include(Catch)

#  Phase A2 — common 모듈 단위 테스트
add_executable(test_common test_common.cpp)
target_link_libraries(test_common PRIVATE
    Catch2::Catch2WithMain
    SJH::common               # PUBLIC include = src/ -> "common/common.h" 접근
)
target_compile_features(test_common PRIVATE cxx_std_17)
catch_discover_tests(test_common)

#  Phase A3 — input utils 단위 테스트
# glfw_input_utils.h 는 GLFW/glad 헤더와 GLFW_PRESS/GLFW_MOD_* 상수에 의존.
# constexpr 함수만 다루므로 라이브러리 링크는 불필요하고 헤더 경로만 필요 —
# glad::glad / glfw 타겟의 INTERFACE_INCLUDE_DIRECTORIES 가 헤더 경로 전파.
add_executable(test_glfw_utils test_glfw_utils.cpp)
target_link_libraries(test_glfw_utils PRIVATE
    Catch2::Catch2WithMain
    glad::glad
    glfw
)
target_include_directories(test_glfw_utils PRIVATE
    ${CMAKE_SOURCE_DIR}/include   # input/glfw_input_utils.h 접근
)
target_compile_features(test_glfw_utils PRIVATE cxx_std_17)
catch_discover_tests(test_glfw_utils)

#  Phase B1 — Headless GL 컨텍스트 fixture (재사용 STATIC lib)
# 향후 GL 상태 쿼리 테스트들이 공유하는 인프라.
# PUBLIC link 로 glad/glfw 헤더 경로 전파 → 소비 테스트가 #include <glad/glad.h> 가능.
add_library(gl_test_fixture STATIC support/gl_test_fixture.cpp)
target_link_libraries(gl_test_fixture PUBLIC
    glad::glad
    glfw
)
target_include_directories(gl_test_fixture PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}   # "support/gl_test_fixture.h" 접근
)
target_compile_features(gl_test_fixture PRIVATE cxx_std_17)

#  Phase B1 — fixture 자가검증
add_executable(test_gl_fixture test_gl_fixture.cpp)
target_link_libraries(test_gl_fixture PRIVATE
    Catch2::Catch2WithMain
    gl_test_fixture
)
target_compile_features(test_gl_fixture PRIVATE cxx_std_17)
catch_discover_tests(test_gl_fixture)

#  diagnostics 모듈 — GLDebug::CheckGL* 7개 함수 동작 검증
add_executable(test_gl_debug test_gl_debug.cpp)
target_link_libraries(test_gl_debug PRIVATE
    Catch2::Catch2WithMain
    gl_test_fixture
    SJH::diagnostics
)
target_compile_features(test_gl_debug PRIVATE cxx_std_17)
catch_discover_tests(test_gl_debug)

#  diagnostics 모듈 — UniformDiagnostics warn-once 트래커 smoke
# GL context 불필요 (spdlog 만 호출).
add_executable(test_uniform_diagnostics test_uniform_diagnostics.cpp)
target_link_libraries(test_uniform_diagnostics PRIVATE
    Catch2::Catch2WithMain
    SJH::diagnostics
)
target_compile_features(test_uniform_diagnostics PRIVATE cxx_std_17)
catch_discover_tests(test_uniform_diagnostics)

#  program 모듈 — ProgramUniforms location 캐시 + setter 동작
# 인라인 GLSL 컴파일/링크 → 실제 program 핸들로 ProgramUniforms 생성.
add_executable(test_program_uniforms test_program_uniforms.cpp)
target_link_libraries(test_program_uniforms PRIVATE
    Catch2::Catch2WithMain
    gl_test_fixture
    SJH::program
)
target_compile_features(test_program_uniforms PRIVATE cxx_std_17)
catch_discover_tests(test_program_uniforms)

#  buffer 모듈 — Buffer (VBO/EBO 통합 RAII) 검증
add_executable(test_buffer test_buffer.cpp)
target_link_libraries(test_buffer PRIVATE
    Catch2::Catch2WithMain
    gl_test_fixture
    SJH::buffer
)
target_compile_features(test_buffer PRIVATE cxx_std_17)
catch_discover_tests(test_buffer)

#  layout 모듈 — VertexLayout (VAO + attribute setter) 검증
add_executable(test_vertex_layout test_vertex_layout.cpp)
target_link_libraries(test_vertex_layout PRIVATE
    Catch2::Catch2WithMain
    gl_test_fixture
    SJH::layout
    SJH::buffer    # TrySetAttrib 테스트가 VBO 도 바인딩
)
target_compile_features(test_vertex_layout PRIVATE cxx_std_17)
catch_discover_tests(test_vertex_layout)
