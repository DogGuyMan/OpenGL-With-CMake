#include "context.h"
#include "buffer/buffer.h"
#include "diagnostics/gl_log.h"
#include "diagnostics/gl_state_log.h" // Task 5 — Init() 끝에 1회 상태 덤프
#include "diagnostics/gl_validate.h"  // GLValidate — Mesh/Shader 정합성 진단
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
            if (ImGui::CollapsingHeader("dirLight"))
            {
                ImGui::DragFloat3("dir.direction", glm::value_ptr(mDirLight.mDirection), 0.01f);
                ImGui::ColorEdit3("dir.ambient", glm::value_ptr(mDirLight.mAmbient));
                ImGui::ColorEdit3("dir.diffuse", glm::value_ptr(mDirLight.mDiffuse));
                ImGui::ColorEdit3("dir.specular", glm::value_ptr(mDirLight.mSpecular));
            }

            for (int i = 0; i < 2; ++i)
            {
                ImGui::PushID(i); // 같은 라벨 충돌 방지
                std::string header = "pointLight[" + std::to_string(i) + "]";
                if (ImGui::CollapsingHeader(header.c_str()))
                {
                    ImGui::DragFloat3("p.position", glm::value_ptr(mPointLights[i].mPos), 0.01f);
                    ImGui::DragFloat("p.distance", &mPointLights[i].mDistance, 0.5f, 1.0f, 3250.0f);
                    ImGui::ColorEdit3("p.ambient", glm::value_ptr(mPointLights[i].mAmbient));
                    ImGui::ColorEdit3("p.diffuse", glm::value_ptr(mPointLights[i].mDiffuse));
                    ImGui::ColorEdit3("p.specular", glm::value_ptr(mPointLights[i].mSpecular));
                }
                ImGui::PopID();
            }

            if (ImGui::CollapsingHeader("spotLight", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat3("s.position", glm::value_ptr(mSpotLight.mPos), 0.01f);
                ImGui::DragFloat3("s.direction", glm::value_ptr(mSpotLight.mDirection), 0.01f);
                ImGui::DragFloat("s.cutoff(deg)", &mSpotLight.mCutoffAngleDeg, 0.1f, 0.0f, 89.0f);
                ImGui::DragFloat("s.outerCutoff(deg)", &mSpotLight.mOuterCutoffAngleDeg, 0.1f, 0.0f, 90.0f);
                ImGui::DragFloat("s.distance", &mSpotLight.mDistance, 0.5f, 1.0f, 3250.0f);
                ImGui::ColorEdit3("s.ambient", glm::value_ptr(mSpotLight.mAmbient));
                ImGui::ColorEdit3("s.diffuse", glm::value_ptr(mSpotLight.mDiffuse));
                ImGui::ColorEdit3("s.specular", glm::value_ptr(mSpotLight.mSpecular));
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

        // 2. Use Program — 광원 *위치 표시 큐브* 들 (DirLight 는 방향만 가지므로 표시 안 함)
        mSimpleProgram->Use();
        {
            // 두 점광원 + 스포트라이트 = 총 3개 마커 큐브. 각자 자기 diffuse 색으로 출력.
            const glm::vec3 markerPositions[3] = {
                mPointLights[0].mPos,
                mPointLights[1].mPos,
                mSpotLight.mPos,
            };
            const glm::vec3 markerColors[3] = {
                mPointLights[0].mDiffuse,
                mPointLights[1].mDiffuse,
                mSpotLight.mDiffuse,
            };
            for (int i = 0; i < 3; ++i)
            {
                auto markerTransform =
                    glm::translate(glm::mat4(1.0f), markerPositions[i]) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
                Uniforms::SetVec4(*mSimpleProgram.get(), "baseColor", glm::vec4(markerColors[i], 1.0f));
                Uniforms::SetMat4(*mSimpleProgram.get(), "transformMat", projMat * viewMat * markerTransform);
                mBox->Draw();
            }
        }

        mProgram->Use();
        {
            Uniforms::SetVec3(*mProgram.get(), "viewPos", mCamera.mPos);

            // --- DirLight 1개 (평행광 / 거리감쇠 없음) ---
            Uniforms::SetVec3(*mProgram.get(), "dirLight.direction", mDirLight.mDirection);
            Uniforms::SetVec3(*mProgram.get(), "dirLight.ambient",   mDirLight.mAmbient);
            Uniforms::SetVec3(*mProgram.get(), "dirLight.diffuse",   mDirLight.mDiffuse);
            Uniforms::SetVec3(*mProgram.get(), "dirLight.specular",  mDirLight.mSpecular);

            // --- PointLight 2개 (배열, 거리감쇠 vec3 는 mDistance 로 도출) ---
            for (int i = 0; i < 2; ++i)
            {
                std::string base = "pointLights[" + std::to_string(i) + "].";
                Uniforms::SetVec3(*mProgram.get(), (base + "position").c_str(),    mPointLights[i].mPos);
                Uniforms::SetVec3(*mProgram.get(), (base + "attenuation").c_str(), GetAttenuationCoeff(mPointLights[i].mDistance));
                Uniforms::SetVec3(*mProgram.get(), (base + "ambient").c_str(),     mPointLights[i].mAmbient);
                Uniforms::SetVec3(*mProgram.get(), (base + "diffuse").c_str(),     mPointLights[i].mDiffuse);
                Uniforms::SetVec3(*mProgram.get(), (base + "specular").c_str(),    mPointLights[i].mSpecular);
            }

            // --- SpotLight 1개 (점광원 + 콘 cutoff) ---
            Uniforms::SetVec3 (*mProgram.get(), "spotLight.position",    mSpotLight.mPos);
            Uniforms::SetVec3 (*mProgram.get(), "spotLight.direction",   mSpotLight.mDirection);
            // CPU 는 degree 로 보관, 셰이더는 cosine 으로 비교 — 송신 시점에 변환.
            Uniforms::SetFloat(*mProgram.get(), "spotLight.cutoff",      cosf(glm::radians(mSpotLight.mCutoffAngleDeg)));
            Uniforms::SetFloat(*mProgram.get(), "spotLight.outerCutoff", cosf(glm::radians(mSpotLight.mOuterCutoffAngleDeg)));
            Uniforms::SetVec3 (*mProgram.get(), "spotLight.attenuation", GetAttenuationCoeff(mSpotLight.mDistance));
            Uniforms::SetVec3 (*mProgram.get(), "spotLight.ambient",     mSpotLight.mAmbient);
            Uniforms::SetVec3 (*mProgram.get(), "spotLight.diffuse",     mSpotLight.mDiffuse);
            Uniforms::SetVec3 (*mProgram.get(), "spotLight.specular",    mSpotLight.mSpecular);

            // --- Material — diffuse/specular 텍스처 unit + shininess ---
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
                mBox->Draw();
            }
        }

        // glPointSize(50.0f);
        // glDrawArrays(GL_POINTS, 0, 1);
    }

    bool Context::Init()
    {
        // === Light Casters 초기화 (사용자 제공 reference 값) ===
        // 거리감쇠 c1=0.09 / c2=0.032 → learnopengl 표 distance≈50 행에 대응하므로 mDistance=50 로 설정.
        // (project 의 GetAttenuationCoeff 가 distance→(Kc,Kl,Kq) 변환을 담당)

        mDirLight.mDirection = glm::vec3(0.0f, -1.0f, 0.0f);
        mDirLight.mAmbient   = glm::vec3(1.0f, 1.0f, 1.0f);
        mDirLight.mDiffuse   = glm::vec3(1.0f, 1.0f, 1.0f);
        mDirLight.mSpecular  = glm::vec3(1.0f, 1.0f, 1.0f);

        mPointLights[0].mPos      = glm::vec3(1.2f, 1.0f, 1.0f);
        mPointLights[0].mDistance = 50.0f;
        mPointLights[0].mAmbient  = glm::vec3(0.05f, 0.05f, 0.05f);
        mPointLights[0].mDiffuse  = glm::vec3(0.8f, 0.4f, 0.2f);
        mPointLights[0].mSpecular = glm::vec3(1.0f, 1.0f, 1.0f);

        mPointLights[1].mPos      = glm::vec3(-1.2f, 1.0f, -1.0f);
        mPointLights[1].mDistance = 50.0f;
        mPointLights[1].mAmbient  = glm::vec3(0.05f, 0.05f, 0.05f);
        mPointLights[1].mDiffuse  = glm::vec3(0.2f, 0.4f, 0.8f);
        mPointLights[1].mSpecular = glm::vec3(1.0f, 1.0f, 1.0f);

        mSpotLight.mPos                  = glm::vec3(0.0f, 1.5f, 0.0f);
        mSpotLight.mDirection            = glm::vec3(0.0f, -1.0f, 0.0f);
        mSpotLight.mCutoffAngleDeg       = 12.5f;
        mSpotLight.mOuterCutoffAngleDeg  = 17.5f;
        mSpotLight.mDistance             = 50.0f;
        mSpotLight.mAmbient              = glm::vec3(0.0f, 0.0f, 0.0f);
        mSpotLight.mDiffuse              = glm::vec3(1.0f, 1.0f, 1.0f);
        mSpotLight.mSpecular             = glm::vec3(1.0f, 1.0f, 1.0f);

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

        // 큐브 메시 1개 — VAO/VBO/EBO 는 Mesh 가 캡슐화. unbind 도 Mesh::Init 내부에서 처리됨.
        // (직전: 여기서 glBindVertexArray(0) / glBindBuffer 3종을 직접 호출했지만 Mesh 캡슐화 후 redundant.
        //  마찬가지로 unit 0/1 에 white 텍스처를 바인딩하고 lighting.fs 의 tex0/tex1 sampler 에 SetInt 까지 했지만,
        //  multi-light 마이그레이션 후 두 sampler 가 셰이더에서 제거되었고 simple.fs 는 sampler 자체가 없음 →
        //  unit 0/1 바인딩, tex0/tex1 SetInt, checkerboard/awesomeface 주석 블록 모두 dead code 로 일괄 제거.)
        mBox = Mesh::CreateBox();

        // Init 끝 — Mesh/program/textures 모두 설정 완료 시점의 *온전한 GL 상태* 1회 덤프.
        // Render() 안에서 의도치 않은 상태 변화가 의심되면 본 baseline과 비교 가능.
        // (매 프레임 호출 금지 — glGet* stall. Init() 1회 한정.)
        Diagnostics::GLStateLog::Dump("Context::Init done");

        // GLValidate program-side 진단 — Mesh ↔ Shader contract.
        // Cat A (CheckIndices) 는 Mesh::Init 이 이미 호출. 본 시점 직전에 Mesh::Draw 가
        // VAO 를 Bind 해야 Cat B (attribute layout) 가 정확 — 명시적으로 Bind 후 검증.
        // Cat C (CheckUniformCoverage) 는 Init 직후 모든 uniform이 default-0 인 게 정상이라 skip
        // (Render 안 setter 호출 후 의미 있음).
        mBox->GetVertexLayout()->Bind();
        mProgram->Use();
        Diagnostics::GLValidate::CheckAttribLayout(mProgram->GetProgramAddr(), "Context::Init");
        Diagnostics::GLValidate::CheckSamplerBindings(mProgram->GetProgramAddr(), "Context::Init");
        Diagnostics::GLValidate::DumpShaderInfoLogs(mProgram->GetProgramAddr(), "Context::Init");

        return true;
    }
}
