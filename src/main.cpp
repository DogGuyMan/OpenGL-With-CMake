#include "common.h"
#include "shader.h"
#include "program.h"

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
    
    /*** 쉐이더 인스턴스 생성 ***/
    
    // UPtr이 아니고 Ptr로 해야지 Program::Create에서 사용 가능한 소유권을 얻는다.
        // ShaderUPtr vertShader = Shader::CreateFromFile("./shader/simple.vs", GL_VERTEX_SHADER);
        // ShaderUPtr fragShader = Shader::CreateFromFile("./shader/simple.fs", GL_FRAGMENT_SHADER); 

    ShaderPtr vertShader = Shader::CreateFromFile("./shader/simple.vs", GL_VERTEX_SHADER);
    ShaderPtr fragShader = Shader::CreateFromFile("./shader/simple.fs", GL_FRAGMENT_SHADER);
    // [2024-11-21 02:47:05.896] [info] [main.cpp:97] vertex shader id : 1
    // [2024-11-21 02:47:05.896] [info] [main.cpp:98] fragment shader id : 2
    SPDLOG_INFO("vertex shader id : {}", vertShader->Get());
    SPDLOG_INFO("fragment shader id : {}", fragShader->Get());

    /*** 프로그램 인스턴스 생성 ***/
    ProgramPtr program = Program::Create({fragShader, vertShader});
    // [2024-11-21 02:47:05.896] [info] [main.cpp:102] program id : 3
    SPDLOG_INFO("program id : {}", program->Get());

    /*** 스크린 배경색 세팅 ***/
    glClearColor(0.0, 0.1f, 0.2f, 0.0f);

    /*** OpenGL 스크린 사이즈 및 키보드 콜백 ***/
    OnFramebufferSizeChange(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSetFramebufferSizeCallback(window, OnFramebufferSizeChange);
    glfwSetKeyCallback(window, OnKeyEvent);
    
    /*** GLFW 루프 시작, 윈도우 close 버튼을 누르면 루프 종료 ***/
    SPDLOG_INFO("Start GLFW main loop"); // [2024-10-11 16:03:55.099] [info] [main.cpp:27] Start GLFW main loop
    while (!glfwWindowShouldClose(window))
    {
        // 윈도우의 크기가 변경되었을 때
        // 윈도우에 마우스 입력이 들어왔을 때
        // 윈도우에 키보드 입력이 들어왔을 때
        // 콜백 수행부
        glfwPollEvents(); // ??
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
    }


    /*** GLFW 종료 호출 전에 만들어 놨던 Shader, Program의 메모리를 한꺼번에 정리를 해야한다. ***/

    // [2024-10-11 16:04:33.541] [info] [main.cpp:32] Terminate GLFW
    SPDLOG_INFO("Terminate GLFW"); 
    glfwTerminate();
    return 0;
}