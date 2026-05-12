#include "context.h"
#include "buffer/buffer.h"
#include "diagnostics/gl_log.h"
#include "diagnostics/gl_state_log.h" // Task 5 — Init() 끝에 1회 상태 덤프
#include "layout/vertex_layout.h"
#include "program/program_uniforms.h" // program.h 가 더 이상 transitive 제공 안 함
#include <imgui.h>
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

    // clang-format off
    float vertices[] = {
        // position(x, y, z)   normal(nx, ny, nz)   color(r, g, b)       uv(u, v)
        // -- back face (z = -0.5, normal -Z) --
        -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,  1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,  1.0f, 1.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,  1.0f, 1.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,  1.0f, 1.0f, 1.0f,   0.0f, 1.0f,
        // -- front face (z = +0.5, normal +Z) --
        -0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,  1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,  1.0f, 1.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,  1.0f, 1.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,  1.0f, 1.0f, 1.0f,   0.0f, 1.0f,
        // -- left face (x = -0.5, normal -X) --
        -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,   1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,   1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
        // -- right face (x = +0.5, normal +X) --
         0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,    1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,   1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,   0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,    1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
        // -- bottom face (y = -0.5, normal -Y) --
        -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,  1.0f, 1.0f, 1.0f,   0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,  1.0f, 1.0f, 1.0f,   1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,  1.0f, 1.0f, 1.0f,   1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,  1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
        // -- top face (y = +0.5, normal +Y) --
        -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,  1.0f, 1.0f, 1.0f,   0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,  1.0f, 1.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,  1.0f, 1.0f, 1.0f,   1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,  1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
    };

    // ! ⭐️ 제발 인덱스 버퍼는 GLuint로 두자 ⭐️
    uint32_t indices[] = {
         0,  2,  1,    2,  0,  3,   // back
         4,  5,  6,    6,  7,  4,   // front
         8,  9, 10,   10, 11,  8,   // left
        12, 14, 13,   14, 12, 15,   // right
        16, 17, 18,   18, 19, 16,   // bottom
        20, 22, 21,   22, 20, 23,   // top
    };
    // clang-format on

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
        const auto cameraFront = mCamera.GetFront(); // 매 프레임 1회만 계산 — 재사용

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
        mCamera.SetAspect((float)width, (float)height); // height==0 가드 내장
    }

    void Context::MouseMove(double x, double y)
    {
        if (!mCamera.mIsCamControl)
            return;
        auto pos = glm::vec2((float)x, (float)y);
        auto deltaPos = pos - mPrevMousePos;

        const float cameraRotSpeed = -0.1f;
        mCamera.mEulerYaw -= deltaPos.x * cameraRotSpeed;
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
        const char *btnName = (button == GLFW_MOUSE_BUTTON_LEFT)     ? "LEFT"
                              : (button == GLFW_MOUSE_BUTTON_RIGHT)  ? "RIGHT"
                              : (button == GLFW_MOUSE_BUTTON_MIDDLE) ? "MIDDLE"
                                                                     : "OTHER";
        const char *actName = (action == GLFW_PRESS) ? "PRESS" : "RELEASE";
        spdlog::info("[MouseButton] {} {} at ({:.1f}, {:.1f})", btnName, actName, x, y);

        if (button == GLFW_MOUSE_BUTTON_RIGHT)
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
        if (ImGui::Begin("ui window"))
        {
            if (ImGui::CollapsingHeader("light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat3("l.direction", glm::value_ptr(mLight.mDirection), 0.01f);
                ImGui::ColorEdit3("l.ambient", glm::value_ptr(mLight.mAmbient));
                ImGui::ColorEdit3("l.diffuse", glm::value_ptr(mLight.mDiffuse));
                ImGui::ColorEdit3("l.specular", glm::value_ptr(mLight.mSpecular));
            }

            if (ImGui::CollapsingHeader("material", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // ImGui 는 float* 를 요구 — 임시 변수로 편집 후 setter 로 clamp 적용.
                float shininess = mMaterial.GetShininess();
                if (ImGui::DragFloat("m.shininess", &shininess, 1.0f, 2.0f, 256.0f))
                    mMaterial.SetShininess(shininess);
            }
            ImGui::Checkbox("animation", &mAnimation);
            // 함수 하나가 UI component 하나에 대응
            // 리턴 값이 true인 경우 해당 UI가 조작되었음을 의미
            // UI 조작 이벤트에 대한 액션 로직을 if으로 작성할 수 있음
            if (ImGui::ColorEdit4("clear color", glm::value_ptr(mClearColor)))
            {
                glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
            }
            ImGui::Separator();
            ImGui::DragFloat3("camera pos", glm::value_ptr(mCamera.mPos), 0.01f);
            ImGui::DragFloat("camera yaw", &mCamera.mEulerYaw, 0.5f);
            ImGui::DragFloat("camera pitch", &mCamera.mEulerPitch, 0.5f, -89.0f, 89.0f);
            ImGui::Separator();
            if (ImGui::Button("reset camera"))
            {
                mCamera.mEulerYaw = 0.0f;
                mCamera.mEulerPitch = 0.0f;
                mCamera.mPos = glm::vec3(0.0f, 0.0f, 3.0f);
            }
        }
        ImGui::End();
        float t = sinf((float)glfwGetTime()) * 0.5f + 0.5f;

        // 카메라: z=3 위치에서 원점을 바라봄. 인자 없는 const 게터 — 멤버 직접 사용.
        auto viewMat = mCamera.GetForwardViewMatrix();
        auto projMat = mCamera.GetProjMatrix(); // mAspect 멤버 사용 (Reshape 에서 갱신)

        // glm::vec4 baseColor(t * t, 2.0f * t * (1.0f - t), (1.0f - t) * (1.0f - t), 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // 2. Use Program
        // mSimpleProgram->Use();
        // mVertexArrayObject->Bind(); // 이거 없으면 안되더라..
        // {
        //     auto lightModelTransform =
        //         glm::translate(glm::mat4(1.0), mLight.mPos) *
        //         glm::scale(glm::mat4(1.0), glm::vec3(0.1f));
        //     Uniforms::SetVec4(*mSimpleProgram.get(), "baseColor", glm::vec4(mLight.mAmbient + mLight.mDiffuse, 1.0f));
        //     Uniforms::SetMat4(*mSimpleProgram.get(), "transformMat", projMat * viewMat * lightModelTransform);
        //     glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        // }

        mProgram->Use();
        mVertexArrayObject->Bind(); // 이거 없으면 안되더라..
        {
            Uniforms::SetVec3(*mProgram.get(), "viewPos", mCamera.mPos);
            Uniforms::SetVec4(*mProgram.get(), "baseColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            Uniforms::SetVec3(*mProgram.get(), "light.direction", mLight.mDirection);
            Uniforms::SetVec3(*mProgram.get(), "light.ambient", mLight.mAmbient);
            Uniforms::SetVec3(*mProgram.get(), "light.diffuse", mLight.mDiffuse);
            Uniforms::SetVec3(*mProgram.get(), "light.specular", mLight.mSpecular);
            // material.specular 는 sampler2D — texture unit index(int).
            // unit 3 에 container2_specular(금속 가장자리만 밝음) 를 바인딩하여
            // 표면의 specular 마스크 역할을 하게 한다.
            Uniforms::SetInt(*mProgram.get(), "material.diffuse", 2);
            Uniforms::SetInt(*mProgram.get(), "material.specular", 3);
            Uniforms::SetFloat(*mProgram.get(), "material.shininess", mMaterial.GetShininess());

            glActiveTexture(GL_TEXTURE2);
            mRM->LoadTextureWithName("container2")->Bind();
            glActiveTexture(GL_TEXTURE3);
            mRM->LoadTextureWithName("container2_specular")->Bind();

            // 3. Uniform 전달
            for (size_t i = 0; i < cubePositions.size(); i++)
            {
                auto &pos = cubePositions[i];
                auto modelMat = glm::translate(glm::mat4(1.0f), pos);
                auto angle = glm::radians((float)glfwGetTime() * 120.0f + 20.0f * (float)i);
                modelMat = glm::rotate(modelMat,
                                       mAnimation ? angle : 0.0f,
                                       glm::vec3(1.0f, 0.5f, 0.0f));
                auto transformMat = projMat * viewMat * modelMat;
                Uniforms::SetMat4(*mProgram.get(), "transformMat", transformMat);
                Uniforms::SetMat4(*mProgram.get(), "modelTransformMat", modelMat);
                // VBO + EBO 협력으로 그렸을때.
                glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            }
        }

        // glPointSize(50.0f);
        // glDrawArrays(GL_POINTS, 0, 1);
    }

    bool Context::Init()
    {

        mProgram = Program::CreateWithVSFS("./resources/shader/lighting.vs", "./resources/shader/lighting.fs");
        if (!mProgram)
            return false;

        mSimpleProgram = Program::CreateWithVSFS("./resources/shader/simple.vs", "./resources/shader/simple.fs");
        if (!mSimpleProgram)
            return false;

        spdlog::info("program id: {}", mProgram->GetProgramAddr());
        glClearColor(0.0, 0.1f, 0.2f, 0.0f);

        mRM = ResourceManagement::CreateRM();
        auto imagePtr1 = mRM->LoadImage("container", "./resources/texture/container.jpg");
        if (imagePtr1 == nullptr)
            return false;
        mRM->LoadTextureFromImage(imagePtr1);

        auto imagePtr2 = mRM->LoadImage("awesomeface", "./resources/texture/awesomeface.png");
        if (imagePtr2 == nullptr)
            return false;
        mRM->LoadTextureFromImage(imagePtr2);

        auto imagePtr3 = mRM->LoadImage("container2", "./resources/texture/container2.png");
        if (imagePtr3 == nullptr)
            return false;
        mRM->LoadTextureFromImage(imagePtr3);

        auto imagePtr4 = mRM->LoadImage("container2_specular", "./resources/texture/container2_specular.png");
        if (imagePtr4 == nullptr)
            return false;
        mRM->LoadTextureFromImage(imagePtr4);

        // 디퓨즈 + 스페큘러 이름 키를 한 번에 설정 (이전: mDiffuseTextureName 에 specular 값을 두 번 대입하던 버그 수정).
        mMaterial.SetTextureNames("container2", "container2_specular");

        auto checkerImgPtr = Image::Create("checkerboard", 512, 512);
        checkerImgPtr->SetCheckImage(16, 16);
        mRM->LoadTextureFromImage(checkerImgPtr.get());

        auto whiteImgPtr = Image::Create("white", 32, 32);
        whiteImgPtr->SetWhiteImage();
        mRM->LoadTextureFromImage(whiteImgPtr.get());

        /*
        순서
            1. VAO
            2. VBO
            3. Vertex Attribute Setting
        */

        // 1. VAO 사용할 buffer object는 vertex data를 저장할 용도
        //      변경빈도(STATIC=한번 / DYNAMIC=자주 / STREAM=매프레임) x 용도(DRAW=앱-> GPU / READ=GPU -> 앱 / COPY=GPU <-> GPU)
        mVertexArrayObject = VertexLayout::Create();
        mVertexBufferObject = Buffer::CreateWithData(GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertices, sizeof(vertices));
        mVertexArrayObject->TrySetAttrib(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, 0);
        mVertexArrayObject->TrySetAttrib(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, sizeof(float) * 3);
        mVertexArrayObject->TrySetAttrib(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, sizeof(float) * 6);
        mVertexArrayObject->TrySetAttrib(3, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 11, sizeof(float) * 9);

        // 2.
        mElementBufferObject = Buffer::CreateWithData(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, indices, sizeof(indices));

        // VBO 및 버텍스 속성을 다 했으니 VBO와 VAO를 unbind한다.
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // 3
        // {
        //     // glActiveTexture(textureSlot) 함수로 현재 다루고자 하는 텍스처 슬롯을 선택
        //     glActiveTexture(GL_TEXTURE0);
        //     // glBindTexture(textureType, textureId) 함수로 현재 설정중인 텍스처 슬롯에 우리의 텍스처 오브젝트를 바인딩
        //     mRM->LoadTextureWithName("checkerboard")
        //         ->Bind(); // -> glBindTexture(GL_TEXTURE_2D, mTextureID);
        // }
        // {
        //     // glActiveTexture(textureSlot) 함수로 현재 다루고자 하는 텍스처 슬롯을 선택
        //     glActiveTexture(GL_TEXTURE1);
        //     // glBindTexture(textureType, textureId) 함수로 현재 설정중인 텍스처 슬롯에 우리의 텍스처 오브젝트를 바인딩
        //     mRM->LoadTextureWithName("awesomeface")
        //         ->Bind(); // -> glBindTexture(GL_TEXTURE_2D, mTextureID);
        // }
        glActiveTexture(GL_TEXTURE0);
        mRM->LoadTextureWithName("white")->Bind();
        glActiveTexture(GL_TEXTURE1);
        mRM->LoadTextureWithName("white")->Bind();

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

        // Init 끝 — VAO/VBO/EBO/program/textures 모두 설정 완료 시점의 *온전한 GL 상태* 1회 덤프.
        // Render() 안에서 의도치 않은 상태 변화가 의심되면 본 baseline과 비교 가능.
        // (매 프레임 호출 금지 — glGet* stall. Init() 1회 한정.)
        Diagnostics::GLStateLog::Dump("Context::Init done");

        return true;
    }
}
