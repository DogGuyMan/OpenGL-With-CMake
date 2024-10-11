#include <iostream>
#include <spdlog/spdlog.h>
#include <GLFW/glfw3.h>

int main(int argc, char *argv[]) {
    SPDLOG_INFO("Hello, OpenGL"); // [2024-10-11 16:03:54.849] [info] [main.cpp:6] Hello, OpenGL

    // GLFW 라이브러리 초기화, 실패하면 에러 메시지 출력
    SPDLOG_INFO("Initialize GLFW"); // [2024-10-11 16:03:54.850] [info] [main.cpp:9] Initialize GLFW
    if(!glfwInit()) {
        const char* description = nullptr;
        glfwGetError(&description);
        SPDLOG_ERROR("failed to initialize GLFW: {}", description);
        return -1;
    }

    // GLFW 윈도우 생성, 실패하면 에러 출력후 종료
    SPDLOG_INFO("Create GLFW window"); // [2024-10-11 16:03:54.977] [info] [main.cpp:18] Create GLFW window
    auto window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, nullptr, nullptr);
    if(!window) {
        SPDLOG_ERROR("failed to create GLFW window");
        glfwTerminate();
        return -1;
    }

    // GLFW 루프 시작, 윈도우 close 버튼을 누르면 루프 종료
    SPDLOG_INFO("Start GLFW main loop"); // [2024-10-11 16:03:55.099] [info] [main.cpp:27] Start GLFW main loop
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    SPDLOG_INFO("Terminate GLFW"); // [2024-10-11 16:04:33.541] [info] [main.cpp:32] Terminate GLFW
    glfwTerminate();
    return 0;
}