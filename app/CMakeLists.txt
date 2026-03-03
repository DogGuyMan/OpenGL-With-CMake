#  Main Application

# cmake/Config.cmake의 WINDOW_* 변수를 헤더로 변환
# CMAKE_CURRENT_SOURCE_DIR = app/ → app/config.h.in 정상 탐색
# CMAKE_CURRENT_BINARY_DIR = build_Darwin/app/ → config.h 출력
configure_file(config.h.in config.h)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    # 외부 의존성
    spdlog::spdlog
    glfw
    glad::glad
    fmt::fmt
    # 내부 모듈 (링크 시 include 경로 자동 전파)
    SJH::shader
    SJH::program
    SJH::buffer
    SJH::common
    SJH::context
    SJH::layout
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}   # 생성된 config.h 탐색 경로
    ${Stb_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include   # 프로젝트 퍼블릭 헤더 루트 (include/input/ 등)
    # SJH::* 모듈 링크 시 src/ include 경로가 INTERFACE로 자동 전파됨
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
