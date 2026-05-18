#include "common/common.h"
#include "config.h"
#include "context/context.h"
#include "diagnostics/gl_state_log.h"  // Task 5 도구 — 시작 시 GL 상태 1회 덤프 + auto-error 정책
#include "diagnostics/gl_validate.h"   // Cat G — viewport ↔ 프레임버퍼 정합 회귀 테스트
#include "input/glfw_input_utils.h"
#include "object/camera.h"
#include "program/program.h"
#include "shader/shader.h"
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <glad/glad.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

void OnKeyEvent(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // GLFW 키 이벤트를 ImGui IO 로 포워딩.
    // ImGui 측에서 io.KeysDown[] 와 modifier(Ctrl/Shift/Alt/Super) 상태를 갱신하므로
    // InputText 입력 / Tab 포커싱 / 단축키(Ctrl+C 등) 가 정상 동작하려면 필수.
    // 누락 시 ImGui 의 키보드 기반 위젯이 모두 무반응이 된다.
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    spdlog::info("key: {} ,scancode: {} ,action: {}, mods: {}{}{}",
                 key, scancode,
                 glfw_utils::ActionToString(action),
                 glfw_utils::ModCtrl(mods),
                 glfw_utils::ModShift(mods),
                 glfw_utils::ModAlt(mods));
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void OnFramebufferSizeChange(GLFWwindow *window, int width, int height)
{
    SPDLOG_INFO("framebuffer size changed: ({} x {})", width, height);
    // glfwSetWindowUserPointer 에 세팅했던것을 가져오는것
    auto context = (SJH::Context *)glfwGetWindowUserPointer(window);
    context->Reshape(width, height);
}

void OnCharEvent(GLFWwindow *window, unsigned int ch)
{
    // OS의 IME/keymap 을 거쳐 변환된 유니코드 codepoint(`ch`)를 ImGui IO 큐에 push.
    // KeyCallback 은 *가상 키 코드* 를 다루는 반면, 이쪽은 실제 *입력된 글자* 를 다루는 별도 채널.
    // InputText 위젯에 글자가 실제로 찍히려면 이 콜백이 반드시 ImGui 로 전달돼야 한다.
    ImGui_ImplGlfw_CharCallback(window, ch);
}

void OnCursorPos(GLFWwindow *window, double x, double y)
{
    // glfwSetWindowUserPointer 에 세팅했던것을 가져오는것
    auto context = (SJH::Context *)glfwGetWindowUserPointer(window);
    context->MouseMove(x, y);
}

void OnMouseButton(GLFWwindow *window, int button, int action, int modifier)
{
    // GLFW 마우스 버튼 PRESS/RELEASE 를 ImGui IO 의 io.MouseDown[] 에 반영.
    // 이 한 줄로 ImGui 가 "지금 자기 위젯 위에서 클릭이 일어났는지" 를 인지하고
    // io.WantCaptureMouse / 위젯 활성·드래그 시작·종료 판단의 근거를 만든다.
    // 누락 시 슬라이더 / 버튼 / ColorEdit 등 모든 ImGui 위젯이 클릭에 반응하지 않는다.
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, modifier);
    // glfwSetWindowUserPointer 에 세팅했던것을 가져오는것
    auto context = (SJH::Context *)glfwGetWindowUserPointer(window);
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    context->MouseButton(button, action, x, y);
}

void OnScroll(GLFWwindow *window, double xoffset, double yoffset)
{
    // 휠 / 트랙패드 스크롤 오프셋을 ImGui IO 의 io.MouseWheel(수직), io.MouseWheelH(수평) 에 누적.
    // DragFloat 휠 미세조정 / 스크롤 영역 / ListBox / Combo 펼침 휠 이동 등이 동작하려면 필수.
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
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

    // 4.1 시작 시점 GL 상태 1회 덤프 (Task 5 도구)
    //     디버그 시 "초기 상태가 spec default와 일치하는지" 빠르게 확인 가능.
    SJH::Diagnostics::GLStateLog::Dump("startup");

    // 4.2 KHR_debug 콜백 자동 Dump 활성화 시도.
    //     macOS GL 3.3 core profile은 KHR_debug 미지원 -> 1회 warn 후 no-op (의도된 동작).
    //     Windows/Linux/llvmpipe에서는 GL 에러 발생 시 자동 Dump.
    SJH::Diagnostics::GLStateLog::EnableAutoOnError(true);

    auto imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(imguiContext);
    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init();
    ImGui_ImplOpenGL3_CreateFontsTexture();
    ImGui_ImplOpenGL3_CreateDeviceObjects();

    // 5. 윈도우 유저 인풋 핸들링 바인드
    auto context = SJH::Context::Create();
    if (!context)
    {
        SPDLOG_ERROR("컨텍스트 생성 실패");
        glfwTerminate();
        return -1;
    }

    // context를 Global(전역) 변수로 둘 수 없으니..
    // glfw 컨텍스트에 캡슐되어 있는 임의 사용 메모리 공간에
    // 유저가 사용할 데이터를 주입(?) 할때 사용
    // 나중에 가져올때는 캐스팅을 통해 가져온다.
    glfwSetWindowUserPointer(window, context.get());

    // 초기 Reshape 시드는 *실제 프레임버퍼 크기* 로 — Retina/HiDPI 에서
    // WINDOW_WIDTH/HEIGHT(논리 크기)와 물리 픽셀이 2배 차이나므로 직접 조회한다.
    // (이전: 논리 상수를 넘겨 viewport·FBO 가 800x600 으로 잡혀 풀스크린 패스가 어긋남)
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    OnFramebufferSizeChange(window, fbWidth, fbHeight);
    glfwSetFramebufferSizeCallback(window, OnFramebufferSizeChange);

    // [Test/Cat G] 초기 seed 회귀 가드 — Reshape 시드 직후 GL viewport 가
    //  *실제 프레임버퍼 크기* 와 일치하는지 1회 검증.
    //  Retina 에서 논리 크기(WINDOW_WIDTH/HEIGHT)로 seed 하던 회귀가 재발하면
    //  여기서 Cat G warn 이 떠서 즉시 잡힌다.
    SJH::Diagnostics::GLValidate::CheckViewport(fbWidth, fbHeight, "startup-seed");
    glfwSetKeyCallback(window, OnKeyEvent);
    glfwSetCharCallback(window, OnCharEvent);
    glfwSetCursorPosCallback(window, OnCursorPos);
    glfwSetMouseButtonCallback(window, OnMouseButton);
    glfwSetScrollCallback(window, OnScroll);

    // 6. GLFW 루프 시작, 윈도우 close 버튼을 누르면 루프 종료
    spdlog::info("GLFW 메인 루프 시작");
    while (!glfwWindowShouldClose(window))
    {
        // 유저 인풋 폴링
        glfwPollEvents();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 렌더링
        context->ProcessInput(window);
        context->Render();
        // 프레임버퍼 스왑 코드 호출 "그림이 그려지는 과정이 노출되지 않도록 해줌"
        /*
        화면에 그림을 그리는 과정
        1. 프레임버퍼 2개를 준비 (front / back)
        2. back buffer에 그림 그리기
        3. front와 back을 바꿔치기
        4. 위의 과정을 반복
        */
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    context.reset();

    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(imguiContext);

    spdlog::info("GLFW 종료");
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
