#include <iostream>
#include <spdlog/spdlog.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

/* 
...
[2024-10-17 19:09:12.655] [info] [main.cpp:7] 프레임 버퍼 사이즈가 변경됨 : (1718 X 1114)
[2024-10-17 19:09:12.661] [info] [main.cpp:7] 프레임 버퍼 사이즈가 변경됨 : (1710 X 1154)
[2024-10-17 19:09:12.673] [info] [main.cpp:7] 프레임 버퍼 사이즈가 변경됨 : (1702 X 1184)
[2024-10-17 19:09:12.678] [info] [main.cpp:7] 프레임 버퍼 사이즈가 변경됨 : (1698 X 1202)
[2024-10-17 19:09:12.689] [info] [main.cpp:7] 프레임 버퍼 사이즈가 변경됨 : (1696 X 1214)
[2024-10-17 19:09:12.694] [info] [main.cpp:7] 프레임 버퍼 사이즈가 변경됨 : (1690 X 1242)
[2024-10-17 19:09:12.705] [info] [main.cpp:7] 프레임 버퍼 사이즈가 변경됨 : (1684 X 1256)
...
*/
void OnFramebufferSizeChange(GLFWwindow* window, int width, int height) 
{
    SPDLOG_INFO("프레임 버퍼 사이즈가 변경됨 : ({} X {})", width, height);
    // OpenGL이 그림을 그릴 영역 지정
    glViewport(0,0, width, height);
}

/*
...
[2024-10-17 19:18:54.418] [info] [main.cpp:35] key: 256 ,scancode: 53 ,action: Repeat, mods: ---
[2024-10-17 19:18:54.452] [info] [main.cpp:35] key: 256 ,scancode: 53 ,action: Repeat, mods: ---
[2024-10-17 19:18:54.485] [info] [main.cpp:35] key: 256 ,scancode: 53 ,action: Repeat, mods: ---
[2024-10-17 19:18:54.514] [info] [main.cpp:35] key: 256 ,scancode: 53 ,action: Released, mods: ---
[2024-10-17 19:18:54.827] [info] [main.cpp:35] key: 65 ,scancode: 0 ,action: Pressed, mods: ---
[2024-10-17 19:18:54.900] [info] [main.cpp:35] key: 83 ,scancode: 1 ,action: Pressed, mods: ---
[2024-10-17 19:18:54.975] [info] [main.cpp:35] key: 65 ,scancode: 0 ,action: Released, mods: ---
[2024-10-17 19:18:54.993] [info] [main.cpp:35] key: 68 ,scancode: 2 ,action: Pressed, mods: ---
...
*/
void OnKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods)  {
    SPDLOG_INFO("key: {} ,scancode: {} ,action: {}, mods: {}{}{}",
        key, scancode,
        action == GLFW_PRESS ? "Pressed" :
        action == GLFW_RELEASE ? "Released" :
        action == GLFW_REPEAT ? "Repeat" : "Unknown",
        mods & GLFW_MOD_CONTROL ? "C" : "-",
        mods & GLFW_MOD_SHIFT ? "S" : "-",
        mods & GLFW_MOD_ALT ? "A" : "-"
    );
    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

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
    // https://heinleinsgame.tistory.com/6

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // GLFW 윈도우 생성, 실패하면 에러 출력후 종료

    SPDLOG_INFO("Create glfw window");
    auto window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, nullptr, nullptr);
    if (!window) {
        SPDLOG_ERROR("failed to create glfw window");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // glad 를 활용한 OpenGL 함수를 로딩함
    // 이게 성공하면 OpenGL 함수를 앞으로 사용할 . 수있게됨
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        SPDLOG_ERROR("failed to initialize glad");
        glfwTerminate();
        return -1;
    }
    auto glVersion = glGetString(GL_VERSION);
    // [2024-10-17 18:51:38.050] [info] [main.cpp:42] OpenGL context version: 4.1 Metal - 89.3
    SPDLOG_INFO("OpenGL context version: {}", reinterpret_cast<const char*>(glVersion)); 
    // OpenGL 버젼을 불러올 . 수있음

    OnFramebufferSizeChange(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSetFramebufferSizeCallback(window, OnFramebufferSizeChange);
    glfwSetKeyCallback(window, OnKeyEvent);
    glClearColor(0.0,0.1f,0.2f,0.0f);

    // GLFW 루프 시작, 윈도우 close 버튼을 누르면 루프 종료
    SPDLOG_INFO("Start GLFW main loop"); // [2024-10-11 16:03:55.099] [info] [main.cpp:27] Start GLFW main loop
    while(!glfwWindowShouldClose(window)) {
        // 윈도우의 크기가 변경되었을 때
        // 윈도우에 마우스 입력이 들어왔을 때
        // 윈도우에 키보드 입력이 들어왔을 때 
        // 콜백 수행부
        glfwPollEvents(); // ??
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
    }
    
    SPDLOG_INFO("Terminate GLFW"); // [2024-10-11 16:04:33.541] [info] [main.cpp:32] Terminate GLFW
    glfwTerminate();
    return 0;
}