# ── Main Application ─────────────────────────────────────────

# cmake/Config.cmake의 WINDOW_* 변수를 헤더로 변환
# CMAKE_CURRENT_SOURCE_DIR = app/ → app/config.h.in 정상 탐색
# CMAKE_CURRENT_BINARY_DIR = build_Darwin/app/ → config.h 출력
configure_file(config.h.in config.h)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    spdlog::spdlog
    glfw
    glad::glad
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}   # 생성된 config.h 탐색 경로
    ${Stb_INCLUDE_DIR}
    ${SJH_OPENGL_SRC_DIR}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
