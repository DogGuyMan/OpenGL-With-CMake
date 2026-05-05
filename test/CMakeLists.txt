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
