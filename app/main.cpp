#include "common.h"
#include "shader.h"
#include "program.h"
#include "context.h"

#include <iostream>
#include <spdlog/spdlog.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

void OnFramebufferSizeChange(GLFWwindow *window, int width, int height)
{
    SPDLOG_INFO("프레임 버퍼 사이즈가 변경됨 : ({} X {})", width, height);
    // OpenGL이 그림을 그릴 영역 지정
    glViewport(0, 0, width, height);
}

void OnKeyEvent(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    SPDLOG_INFO("key: {} ,scancode: {} ,action: {}, mods: {}{}{}",
                key,
                scancode,
                action == GLFW_PRESS        ? "Pressed"     :
                action == GLFW_RELEASE      ? "Released"    :
                action == GLFW_REPEAT       ? "Repeat"      :
                                            "Unknown",

                mods & GLFW_MOD_CONTROL     ? "C" : "-",
                mods & GLFW_MOD_SHIFT       ? "S" : "-",
                mods & GLFW_MOD_ALT         ? "A" : "-"
    );

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

int main(int argc, char *argv[])
{
    // [2024-10-11 16:03:54.849] [info] [main.cpp:6] Hello, OpenGL
    SPDLOG_INFO("Hello, OpenGL");


    /*** GLFW 라이브러리 초기화, 실패하면 에러 메시지 출력 ***/
    // [2024-10-11 16:03:54.850] [info] [main.cpp:9] Initialize GLFW
    SPDLOG_INFO("Initialize GLFW");
    if (!glfwInit())
    {
        const char *description = nullptr;
        glfwGetError(&description);
        SPDLOG_ERROR("failed to initialize GLFW: {}", description);
        return -1;
    }
    // https://heinleinsgame.tistory.com/6

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);



    /*** GLFW 윈도우 생성, 실패하면 에러 출력후 종료  ***/
    SPDLOG_INFO("Create glfw window");
    auto window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, nullptr, nullptr);
    if (!window)
    {
        SPDLOG_ERROR("failed to create glfw window");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);



    /*** glad 를 활용한 OpenGL 함수를 로딩함 이게 성공하면 OpenGL 함수를 앞으로 사용할 수있게됨 ***/
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        SPDLOG_ERROR("failed to initialize glad");
        glfwTerminate();
        return -1;
    }
    /*** OpenGL 버젼을 불러올 수 있음 ***/
    auto glVersion = glGetString(GL_VERSION);

    // [2024-11-21 02:47:05.895] [info] [main.cpp:87] OpenGL context version: 4.1 Metal - 89.3
    SPDLOG_INFO("OpenGL context version: {}", reinterpret_cast<const char *>(glVersion));


    /*** 렌더링 파이프라인 프로그램 생성 책임을 수행하는 Context 생성, 초기화 ***/
    ContextPtr context = Context::Create();
    if(!context) {
        SPDLOG_ERROR("failed to create context");
        glfwTerminate();
        return -1;
    }

    /*** OpenGL 스크린 사이즈 및 키보드 콜백 ***/
    OnFramebufferSizeChange(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSetFramebufferSizeCallback(window, OnFramebufferSizeChange);
    glfwSetKeyCallback(window, OnKeyEvent);

    /*** GLFW 루프 시작, 윈도우 close 버튼을 누르면 루프 종료 ***/
    SPDLOG_INFO("Start GLFW main loop"); // [2024-10-11 16:03:55.099] [info] [main.cpp:27] Start GLFW main loop
    while (!glfwWindowShouldClose(window))
    {
        context.get()->Render();
        glfwSwapBuffers(window);
        glfwPollEvents(); // ??
    }

    /*** GLFW 종료 호출 전에 만들어 놨던 Shader, Program의 메모리를 한꺼번에 정리를 해야한다. ***/
    // 유니크 포인터의 reset으로 모두 초기화 해제할 수 있다.
    context.reset();
  //context = nullptr; 혹은 context의 소유권을 null로 만드는 방법도 수행할 수 있다.

    // [2024-10-11 16:04:33.541] [info] [main.cpp:32] Terminate GLFW
    SPDLOG_INFO("Terminate GLFW");
    glfwTerminate();
    return 0;
}
