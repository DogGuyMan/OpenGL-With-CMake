#include "context.h"
#include "buffer/buffer.h"
#include "diagnostics/gl_log.h"
#include "layout/vertex_layout.h"
#include "program/program_uniforms.h"   // program.h 가 더 이상 transitive 제공 안 함
#include <spdlog/spdlog.h>

namespace SJH
{
    /*********************************************************************************
     *
     * ! ⭐️ 제발 인덱스 버퍼는 GLuint로 두자 GLfloat으로 잘못 두지 말고 😭 ⭐️
     * ! ⭐️ 제발 정점은 float으로 놓고 ⭐️
     * ! 이런 것때문에 Buffer 같은 강한 타입으로 클래스 디자인을 해야함.
     *
     *********************************************************************************/
    // 엥? 교안 CW 배치네..
    float vertices[] = {
        -0.5f,
        -0.5f,
        -0.5f,
        1.0,
        1.0,
        1.0,
        0.0f,
        0.0f,
        0.5f,
        -0.5f,
        -0.5f,
        1.0,
        1.0,
        1.0,
        1.0f,
        0.0f,
        0.5f,
        0.5f,
        -0.5f,
        1.0,
        1.0,
        1.0,
        1.0f,
        1.0f,
        -0.5f,
        0.5f,
        -0.5f,
        1.0,
        1.0,
        1.0,
        0.0f,
        1.0f,

        -0.5f,
        -0.5f,
        0.5f,
        1.0,
        1.0,
        1.0,
        0.0f,
        0.0f,
        0.5f,
        -0.5f,
        0.5f,
        1.0,
        1.0,
        1.0,
        1.0f,
        0.0f,
        0.5f,
        0.5f,
        0.5f,
        1.0,
        1.0,
        1.0,
        1.0f,
        1.0f,
        -0.5f,
        0.5f,
        0.5f,
        1.0,
        1.0,
        1.0,
        0.0f,
        1.0f,

        -0.5f,
        0.5f,
        0.5f,
        1.0,
        1.0,
        1.0,
        1.0f,
        0.0f,
        -0.5f,
        0.5f,
        -0.5f,
        1.0,
        1.0,
        1.0,
        1.0f,
        1.0f,
        -0.5f,
        -0.5f,
        -0.5f,
        1.0,
        1.0,
        1.0,
        0.0f,
        1.0f,
        -0.5f,
        -0.5f,
        0.5f,
        1.0,
        1.0,
        1.0,
        0.0f,
        0.0f,

        0.5f,
        0.5f,
        0.5f,
        1.0,
        1.0,
        1.0,
        1.0f,
        0.0f,
        0.5f,
        0.5f,
        -0.5f,
        1.0,
        1.0,
        1.0,
        1.0f,
        1.0f,
        0.5f,
        -0.5f,
        -0.5f,
        1.0,
        1.0,
        1.0,
        0.0f,
        1.0f,
        0.5f,
        -0.5f,
        0.5f,
        1.0,
        1.0,
        1.0,
        0.0f,
        0.0f,

        -0.5f,
        -0.5f,
        -0.5f,
        1.0,
        1.0,
        1.0,
        0.0f,
        1.0f,
        0.5f,
        -0.5f,
        -0.5f,
        1.0,
        1.0,
        1.0,
        1.0f,
        1.0f,
        0.5f,
        -0.5f,
        0.5f,
        1.0,
        1.0,
        1.0,
        1.0f,
        0.0f,
        -0.5f,
        -0.5f,
        0.5f,
        1.0,
        1.0,
        1.0,
        0.0f,
        0.0f,

        -0.5f,
        0.5f,
        -0.5f,
        1.0,
        1.0,
        1.0,
        0.0f,
        1.0f,
        0.5f,
        0.5f,
        -0.5f,
        1.0,
        1.0,
        1.0,
        1.0f,
        1.0f,
        0.5f,
        0.5f,
        0.5f,
        1.0,
        1.0,
        1.0,
        1.0f,
        0.0f,
        -0.5f,
        0.5f,
        0.5f,
        1.0,
        1.0,
        1.0,
        0.0f,
        0.0f,
    };

    // ! ⭐️ 제발 인덱스 버퍼는 GLuint로 두자 ⭐️
    uint32_t indices[] = {
        0,
        2,
        1,
        2,
        0,
        3,
        4,
        5,
        6,
        6,
        7,
        4,
        8,
        9,
        10,
        10,
        11,
        8,
        12,
        14,
        13,
        14,
        12,
        15,
        16,
        17,
        18,
        18,
        19,
        16,
        20,
        22,
        21,
        22,
        20,
        23,
    };

    std::vector<glm::vec3> cubePositions = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(2.0f, 5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f, 3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f, 2.0f, -2.5f),
        glm::vec3(1.5f, 0.2f, -1.5f),
        glm::vec3(-1.3f, 1.0f, -1.5f),
    };

    ContextUPtr Context::Create()
    {
        auto context = ContextUPtr(new Context());
        if (!context->Init())
            return nullptr;
        return std::move(context);
    }
    void Context::ProcessInput(GLFWwindow *window)
    {
        if (!mCamera.mIsCamControl)
            return;
        const float cameraSpeed = 0.05f;
        const auto cameraFront = mCamera.GetFront();   // 매 프레임 1회만 계산 — 재사용

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            mCamera.mPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            mCamera.mPos -= cameraSpeed * cameraFront;

        auto cameraRight = glm::normalize(glm::cross(mCamera.mCamUp, -cameraFront));
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            mCamera.mPos += cameraSpeed * cameraRight;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            mCamera.mPos -= cameraSpeed * cameraRight;

        auto cameraUp = glm::normalize(glm::cross(-cameraFront, cameraRight));
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            mCamera.mPos += cameraSpeed * cameraUp;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            mCamera.mPos -= cameraSpeed * cameraUp;
    }

    void Context::Reshape(int width, int height)
    {
        mWidth = width;
        mHeight = height;
        glViewport(0, 0, mWidth, mHeight);
        mCamera.SetAspect((float)width, (float)height);   // height==0 가드 내장
    }

    void Context::MouseMove(double x, double y)
    {
        if (!mCamera.mIsCamControl)
            return;
        auto pos = glm::vec2((float)x, (float)y);
        auto deltaPos = pos - mPrevMousePos;

        const float cameraRotSpeed = -0.1f;
        mCamera.mEulerYaw   -= deltaPos.x * cameraRotSpeed;
        mCamera.mEulerPitch -= deltaPos.y * cameraRotSpeed;

        if (mCamera.mEulerYaw < 0.0f)
            mCamera.mEulerYaw += 360.0f;
        if (mCamera.mEulerYaw > 360.0f)
            mCamera.mEulerYaw -= 360.0f;

        if (mCamera.mEulerPitch > 89.0f)
            mCamera.mEulerPitch = 89.0f;
        if (mCamera.mEulerPitch < -89.0f)
            mCamera.mEulerPitch = -89.0f;

        mPrevMousePos = pos;
    }

    void Context::MouseButton(int button, int action, double x, double y)
    {
        const char* btnName = (button == GLFW_MOUSE_BUTTON_LEFT)   ? "LEFT"
                            : (button == GLFW_MOUSE_BUTTON_RIGHT)  ? "RIGHT"
                            : (button == GLFW_MOUSE_BUTTON_MIDDLE) ? "MIDDLE" : "OTHER";
        const char* actName = (action == GLFW_PRESS) ? "PRESS" : "RELEASE";
        spdlog::info("[MouseButton] {} {} at ({:.1f}, {:.1f})", btnName, actName, x, y);

        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (action == GLFW_PRESS)
            {
                mPrevMousePos = glm::vec2((float)x, (float)y);
                mCamera.mIsCamControl = true;
                spdlog::info("[MouseButton] IsCamControl=true, mPrevMousePos=({:.1f},{:.1f})", x, y);
            }
            else if (action == GLFW_RELEASE)
            {
                mCamera.mIsCamControl = false;
                spdlog::info("[MouseButton] IsCamControl=false");
            }
        }
    }

    void Context::Render()
    {
        float t = sinf((float)glfwGetTime()) * 0.5f + 0.5f;

        // 카메라: z=3 위치에서 원점을 바라봄. 인자 없는 const 게터 — 멤버 직접 사용.
        auto viewMat = mCamera.GetForwardViewMatrix();
        auto projMat = mCamera.GetProjMatrix();   // mAspect 멤버 사용 (Reshape 에서 갱신)

        glm::vec4 baseColor(t * t, 2.0f * t * (1.0f - t), (1.0f - t) * (1.0f - t), 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // 2. Use Program
        mProgram->Use();
        mVertexArrayObject->Bind();

        // 3. Uniform 전달
        for (size_t i = 0; i < cubePositions.size(); i++)
        {
            auto &pos = cubePositions[i];
            auto modelMat = glm::translate(glm::mat4(1.0f), pos);
            modelMat = glm::rotate(modelMat,
                                   glm::radians((float)glfwGetTime() * 120.0f + 20.0f * (float)i),
                                   glm::vec3(1.0f, 0.5f, 0.0f));
            auto transformMat = projMat * viewMat * modelMat;
            Uniforms::SetMat4(*mProgram.get(), "transformMat", glm::value_ptr(transformMat));
            Uniforms::SetVec4(*mProgram.get(), "baseColor", glm::value_ptr(baseColor));
            // VBO + EBO 협력으로 그렸을때.
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }

        glPointSize(50.0f);
        glDrawArrays(GL_POINTS, 0, 1);
    }

    bool Context::Init()
    {
        // ShaderUPtr -> ShaderPtr 암묵 변환 — Program::Create 가 shared 입력을 요구
        ShaderPtr vertexShader = Shader::CreateFromFile(
            "./resources/shader/simple.vs", GL_VERTEX_SHADER);
        ShaderPtr fragmentShader = Shader::CreateFromFile(
            "./resources/shader/simple.fs", GL_FRAGMENT_SHADER);

        if (!vertexShader || !fragmentShader)
            return false;

        spdlog::info("vertex shader id: {}", vertexShader->GetShaderAddr());
        spdlog::info("fragment shader id: {}", fragmentShader->GetShaderAddr());

        mProgram = Program::Create({vertexShader, fragmentShader});
        if (!mProgram)
            return false;
        spdlog::info("program id: {}", mProgram->GetProgramAddr());
        glClearColor(0.0, 0.1f, 0.2f, 0.0f);

        mRM = ResourceManagement::CreateRM();

        /*
        순서
            1. VAO
            2. VBO
            3. Vertex Attribute Setting
        */

        mVertexArrayObject = VertexLayout::Create();
        mVertexArrayObject->Bind();

        // === buffer object를 만든다 ===
        // === 지금부터 사용할 buffer object를 지정한다. ===
        // === 사용할 buffer object는 vertex data를 저장할 용도 ===
        // === 변경빈도(STATIC=한번 / DYNAMIC=자주 / STREAM=매프레임) x 용도(DRAW=앱-> GPU / READ=GPU -> 앱 / COPY=GPU <-> GPU) ===
        mVertexBufferObject = Buffer::CreateWithData( GL_ARRAY_BUFFER, GL_STATIC_DRAW,vertices, sizeof(vertices));
        mElementBufferObject = Buffer::CreateWithData( GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW,indices, sizeof(indices));

        mVertexArrayObject->TrySetAttrib(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, 0);
        mVertexArrayObject->TrySetAttrib(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, sizeof(float) * 3);
        mVertexArrayObject->TrySetAttrib(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, sizeof(float) * 6);

        auto imagePtr1 = mRM->LoadImage("container", "./resources/texture/container.jpg");
        if (imagePtr1 == nullptr)
            return false;


        auto imagePtr2 = mRM->LoadImage("awesomeface", "./resources/texture/awesomeface.png");
        if (imagePtr2 == nullptr)
            return false;

        auto checkerImgPtr = Image::Create("checkerboard", 512, 512);
        checkerImgPtr->SetCheckImage(16, 16);

        mRM->LoadTextureFromImage(imagePtr1);
        mRM->LoadTextureFromImage(imagePtr2);
        mRM->LoadTextureFromImage(checkerImgPtr.get());

        auto texturePtr = mRM->LoadTextureWithName("awesomeface");

        {
            // glActiveTexture(textureSlot) 함수로 현재 다루고자 하는 텍스처 슬롯을 선택
            glActiveTexture(GL_TEXTURE0);
            // glBindTexture(textureType, textureId) 함수로 현재 설정중인 텍스처 슬롯에 우리의 텍스처 오브젝트를 바인딩
            mRM->LoadTextureWithName("checkerboard")
                ->Bind(); // -> glBindTexture(GL_TEXTURE_2D, mTextureID);
        }
        {
            // glActiveTexture(textureSlot) 함수로 현재 다루고자 하는 텍스처 슬롯을 선택
            glActiveTexture(GL_TEXTURE1);
            // glBindTexture(textureType, textureId) 함수로 현재 설정중인 텍스처 슬롯에 우리의 텍스처 오브젝트를 바인딩
            mRM->LoadTextureWithName("awesomeface")
                ->Bind(); // -> glBindTexture(GL_TEXTURE_2D, mTextureID);
        }

        mProgram->Use();

        for (int i = 0; i < 2; i++)
        {
            std::string texuniform = "tex" + std::to_string(i);
            // glGetUniformLocation() 함수로 shader 내의 sampler2D uniform 핸들을 얻어옴
            //      auto loc = glGetUniformLocation(mProgram->GetProgramAddr(), texuniform.c_str());
            // glUniform1i() 함수로 sampler2D uniform에 텍스처 슬롯 인덱스를 입력
            //      glUniform1i(loc, i);
            Uniforms::SetInt(*mProgram.get(), texuniform.c_str(), i);
        }

        return true;
    }
}
