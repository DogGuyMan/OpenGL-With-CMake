#include "config.h"
#include "input/glfw_input_utils.h"
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <glad/glad.h>
#include <spdlog/spdlog.h>

void HandleFramebufferSizeChange(GLFWwindow* window, int width, int height)
{
    SPDLOG_INFO("프레임 버퍼 사이즈가 변경됨 : ({} X {})", width, height);
    // OpenGL이 그림을 그릴 영역 지정
    glViewport(0,0, width, height);
}

void HandleKeyInput(GLFWwindow* window, int key, int scancode, int action, int mods)  {
    SPDLOG_INFO("key: {} ,scancode: {} ,action: {}, mods: {}{}{}",
        key, scancode,
        glfw_utils::ActionToString(action),
        glfw_utils::ModCtrl(mods),
        glfw_utils::ModShift(mods),
        glfw_utils::ModAlt(mods)
    );
    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    spdlog::info("Welcome, {}!", APP_NAME);

    // 1. GLFW 라이브러리 초기화, 실패하면 에러 메시지 출력
    spdlog::info("Try Initialize GLFW");

    if (!glfwInit())
    {
        const char *errorDescription = nullptr;
        glfwGetError(&errorDescription);
        spdlog::error("Failed to initialize GLFW : {}", errorDescription);
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 2. GLFW 윈도우 생성, 실패하면 에러 출력후 종료
    spdlog::info("Create glfw window");
    auto window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, nullptr, nullptr);
    if (!window)
    {
        spdlog::error("Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 3. glad 를 활용한 OpenGL 함수를 로딩함. 이게 성공하면 OpenGL 함수를 앞으로 사용할 수 있게됨
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        spdlog::error("Failed to initialize GLAD");
        glfwTerminate();
        return -1;
    }

    // 4. OpenGL 버젼 출력
    auto glVersion = glGetString(GL_VERSION);
    spdlog::info("OpenGL context version: {}", reinterpret_cast<const char*>(glVersion));

    // 5. 윈도우 유저 인풋 핸들링 바인드
    HandleFramebufferSizeChange(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSetFramebufferSizeCallback(window, HandleFramebufferSizeChange);
    glfwSetKeyCallback(window, HandleKeyInput);
    glClearColor(0.0, 0.1f, 0.2f, 0.0f);

    // 6. GLFW 루프 시작, 윈도우 close 버튼을 누르면 루프 종료
    spdlog::info("Start GLFW main loop");
    while (!glfwWindowShouldClose(window))
    {
        // TODO 윈도우의 크기가 변경되었을 때
        // TODO 윈도우에 마우스 입력이 들어왔을 때
        // TODO 윈도우에 키보드 입력이 들어왔을 때
        // TODO 콜백 수행부
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
    }

    spdlog::info("Terminate GLFW");
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
