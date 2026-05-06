#include "common/common.h"
#include "config.h"
#include "context/context.h"
#include "input/glfw_input_utils.h"
#include "program/program.h"
#include "shader/shader.h"
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <glad/glad.h> // glad 먼저 — 이후 GLFW 경로 안전
#include <spdlog/spdlog.h>

void HandleFramebufferSizeChange(GLFWwindow *window, int width, int height)
{
    SPDLOG_INFO("프레임 버퍼 사이즈가 변경됨 : ({} X {})", width, height);
    // OpenGL이 그림을 그릴 영역 지정
    glViewport(0, 0, width, height);
}

void HandleKeyInput(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    SPDLOG_INFO("key: {} ,scancode: {} ,action: {}, mods: {}{}{}",
                key, scancode,
                glfw_utils::ActionToString(action),
                glfw_utils::ModCtrl(mods),
                glfw_utils::ModShift(mods),
                glfw_utils::ModAlt(mods));
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    spdlog::info("Welcome, {}!", APP_NAME);

    // 1. GLFW 라이브러리 초기화, 실패하면 에러 메시지 출력
    spdlog::info("GLFW 초기화 시도");

    if (!glfwInit())
    {
        const char *errorDescription = nullptr;
        glfwGetError(&errorDescription);
        spdlog::error("GLFW 초기화 실패 : {}", errorDescription);
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 2. GLFW 윈도우 생성, 실패하면 에러 출력후 종료
    spdlog::info("GLFW 윈도우 생성");
    auto window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, nullptr, nullptr);
    if (!window)
    {
        spdlog::error("GLFW 윈도우 생성 실패");
        glfwTerminate();
        return -1;
    }
    // OpenGL의 철학은 세팅 함수들은 어딘가에 Contex 에다가 데아터를 저장한다.
    // State-setting function 과 State-using function 으로 나뉘고
    // 1. 전자 State가 OpenGL context에 저장됨
    // 2. 후자 OpenGL context에 저장된 State를 이용
    glfwMakeContextCurrent(window);

    // 3. glad 를 활용한 OpenGL 함수를 로딩함. 이게 성공하면 OpenGL 함수를 앞으로 사용할 수 있게됨
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        spdlog::error("GLAD 초기화 실패");
        glfwTerminate();
        return -1;
    }

    // 4. OpenGL 버젼 출력
    auto glVersion = glGetString(GL_VERSION);
    spdlog::info("OpenGL context version: {}", reinterpret_cast<const char *>(glVersion));

    // 5. 윈도우 유저 인풋 핸들링 바인드
    auto context = SJH::Context::Create();
    if (!context)
    {
        SPDLOG_ERROR("컨텍스트 생성 실패");
        glfwTerminate();
        return -1;
    }
    HandleFramebufferSizeChange(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSetFramebufferSizeCallback(window, HandleFramebufferSizeChange);
    glfwSetKeyCallback(window, HandleKeyInput);

    // 6. GLFW 루프 시작, 윈도우 close 버튼을 누르면 루프 종료
    spdlog::info("GLFW 메인 루프 시작");
    while (!glfwWindowShouldClose(window))
    {
        // TODO 윈도우의 크기가 변경되었을 때
        // TODO 윈도우에 마우스 입력이 들어왔을 때
        // TODO 윈도우에 키보드 입력이 들어왔을 때
        // TODO 콜백 수행부

        // 렌더링
        context->Render();
        // 프레임버퍼 스왑 코드 호출 "그림이 그려지는 과정이 노출되지 않도록 해줌"
        /*
        화면에 그림을 그리는 과정
        1. 프레임버퍼 2개를 준비 (front / back)
        2. back buffer에 그림 그리기
        3. front와 back을 바꿔치기
        4. 위의 과정을 반복
        */
        glfwSwapBuffers(window);
        // 유저 인풋 폴링
        glfwPollEvents();
    }
    context.reset();

    spdlog::info("GLFW 종료");
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
