# ── Tests ────────────────────────────────────────────────────
# Catch2 또는 GTest 추가 시 vcpkg.json에 "catch2" / "gtest" 추가 후 아래 주석 해제.

# find_package(Catch2 CONFIG REQUIRED)

# add_executable(test_core test_core.cpp)
# target_link_libraries(test_core PRIVATE Catch2::Catch2WithMain)
# target_include_directories(test_core PRIVATE ${SJH_OPENGL_SRC_DIR})

# include(CTest)
# include(Catch)
# catch_discover_tests(test_core)
