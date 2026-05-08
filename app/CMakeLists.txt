#  Main Application

# cmake/Config.cmake의 WINDOW_* 변수를 헤더로 변환
# CMAKE_CURRENT_SOURCE_DIR = app/ -> app/config.h.in 정상 탐색
# CMAKE_CURRENT_BINARY_DIR = build_Darwin/app/ -> config.h 출력
configure_file(config.h.in config.h)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    # 외부 의존성
    spdlog::spdlog
    glfw
    glad::glad
    glm::glm
    fmt::fmt
    imgui::imgui
    imguizmo::imguizmo
    # 내부 모듈 (링크 시 include 경로 자동 전파)
    SJH::buffer
    SJH::object
    SJH::common
    SJH::context
    SJH::diagnostics
    SJH::layout
    SJH::program
    SJH::shader
    SJH::resource_management
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}   # 생성된 config.h 탐색 경로
    ${Stb_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include   # 프로젝트 퍼블릭 헤더 루트 (include/input/ 등)
    # SJH::* 모듈 링크 시 src/ include 경로가 INTERFACE로 자동 전파됨
)

# ______ 빌드 후 리소스 복사 (프로젝트 루트 기준) ______
if(EXISTS ${CMAKE_SOURCE_DIR}/resources)
    if(WIN32)
        # Windows: symlink 권한 이슈 -> copy_directory_if_different 로 fallback
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                    "${CMAKE_SOURCE_DIR}/resources"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
            BYPRODUCTS "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
            VERBATIM)
    else()
        # Unix: 기존 symlink 정리 후 재생성
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -f
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                    "${CMAKE_SOURCE_DIR}/resources"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
            VERBATIM)
    endif()
endif()
