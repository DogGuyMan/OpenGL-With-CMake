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
        if (!mCamera.IsCamControl)
            return;
        const float cameraSpeed = 0.05f;
        const auto cameraFront = mCamera.GetFront(); // 매 프레임 1회만 계산 — 재사용

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            mCamera.Pos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            mCamera.Pos -= cameraSpeed * cameraFront;

        auto cameraRight = glm::normalize(glm::cross(mCamera.CamUp, -cameraFront));
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            mCamera.Pos += cameraSpeed * cameraRight;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            mCamera.Pos -= cameraSpeed * cameraRight;

        auto cameraUp = glm::normalize(glm::cross(-cameraFront, cameraRight));
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            mCamera.Pos += cameraSpeed * cameraUp;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            mCamera.Pos -= cameraSpeed * cameraUp;
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
        if (!mCamera.IsCamControl)
            return;
        auto pos = glm::vec2((float)x, (float)y);
        auto deltaPos = pos - mPrevMousePos;

        const float cameraRotSpeed = -0.1f;
        mCamera.EulerYaw -= deltaPos.x * cameraRotSpeed;
        mCamera.EulerPitch -= deltaPos.y * cameraRotSpeed;

        if (mCamera.EulerYaw < 0.0f)
            mCamera.EulerYaw += 360.0f;
        if (mCamera.EulerYaw > 360.0f)
            mCamera.EulerYaw -= 360.0f;

        if (mCamera.EulerPitch > 89.0f)
            mCamera.EulerPitch = 89.0f;
        if (mCamera.EulerPitch < -89.0f)
            mCamera.EulerPitch = -89.0f;

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
                mCamera.IsCamControl = true;
                spdlog::info("[MouseButton] IsCamControl=true, mPrevMousePos=({:.1f},{:.1f})", x, y);
            }
            else if (action == GLFW_RELEASE)
            {
                mCamera.IsCamControl = false;
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
                ImGui::Checkbox("dir.enabled", &mDirLightEnabled);
                ImGui::DragFloat3("dir.direction", glm::value_ptr(mDirLight.Direction), 0.01f);
                ImGui::ColorEdit3("dir.ambient", glm::value_ptr(mDirLight.Ambient));
                ImGui::ColorEdit3("dir.diffuse", glm::value_ptr(mDirLight.Diffuse));
                ImGui::ColorEdit3("dir.specular", glm::value_ptr(mDirLight.Specular));
            }

            for (int i = 0; i < 2; ++i)
            {
                ImGui::PushID(i); // 같은 라벨 충돌 방지
                std::string header = "pointLight[" + std::to_string(i) + "]";
                if (ImGui::CollapsingHeader(header.c_str()))
                {
                    ImGui::Checkbox("p.enabled", &mPointLightsEnabled[i]);
                    ImGui::DragFloat3("p.position", glm::value_ptr(mPointLights[i].Pos), 0.01f);
                    ImGui::DragFloat("p.distance", &mPointLights[i].Distance, 0.5f, 1.0f, 3250.0f);
                    ImGui::ColorEdit3("p.ambient", glm::value_ptr(mPointLights[i].Ambient));
                    ImGui::ColorEdit3("p.diffuse", glm::value_ptr(mPointLights[i].Diffuse));
                    ImGui::ColorEdit3("p.specular", glm::value_ptr(mPointLights[i].Specular));
                }
                ImGui::PopID();
            }

            if (ImGui::CollapsingHeader("spotLight", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("s.enabled", &mSpotLightEnabled);
                ImGui::DragFloat3("s.position", glm::value_ptr(mSpotLight.Pos), 0.01f);
                ImGui::DragFloat3("s.direction", glm::value_ptr(mSpotLight.Direction), 0.01f);
                ImGui::DragFloat("s.cutoff(deg)", &mSpotLight.CutoffAngleDeg, 0.1f, 0.0f, 89.0f);
                ImGui::DragFloat("s.outerCutoff(deg)", &mSpotLight.OuterCutoffAngleDeg, 0.1f, 0.0f, 90.0f);
                ImGui::DragFloat("s.distance", &mSpotLight.Distance, 0.5f, 1.0f, 3250.0f);
                ImGui::ColorEdit3("s.ambient", glm::value_ptr(mSpotLight.Ambient));
                ImGui::ColorEdit3("s.diffuse", glm::value_ptr(mSpotLight.Diffuse));
                ImGui::ColorEdit3("s.specular", glm::value_ptr(mSpotLight.Specular));
                ImGui::Checkbox("flash light", &mFlashLightMode);
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
            ImGui::DragFloat3("camera pos", glm::value_ptr(mCamera.Pos), 0.01f);
            ImGui::DragFloat("camera yaw", &mCamera.EulerYaw, 0.5f);
            ImGui::DragFloat("camera pitch", &mCamera.EulerPitch, 0.5f, -89.0f, 89.0f);
            ImGui::Separator();
            if (ImGui::Button("reset camera"))
            {
                mCamera.EulerYaw = 0.0f;
                mCamera.EulerPitch = 0.0f;
                mCamera.Pos = glm::vec3(0.0f, 0.0f, 3.0f);
            }
        }
        ImGui::End();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST); // 깊이 테스트 사용하기
        // glDisable(GL_DEPTH_TEST); // 깊이 테스트 사용하지 않기.
        // glDepthMask(GL_FALSE); // depth buffer의 업데이트 막기
        // glClearDepth(1.0f); // depth buffer의 초기값 설정하기

        /* 사용 가능한 비교 연산자
            GL_ALWAYS, GL_NEVER
            GL_LESS, GL_LEQUAL
            GL_GREATER, GL_GEQUAL
            GL_EQUAL, GL_NOTEQUAL */
        // glDepthFunc(GL_LESS); // depth test 비교 연산자 변경하기

        float t = sinf((float)glfwGetTime()) * 0.5f + 0.5f;

        // 카메라: z=3 위치에서 원점을 바라봄. 인자 없는 const 게터 — 멤버 직접 사용.
        auto viewMat = mCamera.GetForwardViewMatrix();

        {
            /* perspective projection을 적용하면 깊이값을 0~1 사이로 정규화하면서 w값으로 나누는 과정을 거침
            정규화된 z값은 1/z 꼴의 함수 형태로 분포가 나타남

            깊이 값의 왜곡 멀리 있는 픽셀 간에 z값의 오차가 크지 않아서 문제가 발생할 수 있음 z-fighting
            예방법 :
                면과 면을 너무 붙어있게 하지 않을 것
                near의 값을 너무 작게 하지 말것
                좀더 정확한 depth buffer를 설정하여 사용할 것
            */
            // mCamera.FarPlane = 10.f;
            // mCamera.NearPlane = 0.5f;
        }
        auto projMat = mCamera.GetProjMatrix(); // mAspect 멤버 사용 (Reshape 에서 갱신)

        // glm::vec4 baseColor(t * t, 2.0f * t * (1.0f - t), (1.0f - t) * (1.0f - t), 1.0f);

        // 2. Use Program — 광원 *위치 표시 큐브* 들 (DirLight 는 방향만 가지므로 표시 안 함)
        mSimpleProgram->Use();
        {
            // 두 점광원 + 스포트라이트 = 총 3개 마커 큐브. 각자 자기 diffuse 색으로 출력.
            const glm::vec3 markerPositions[3] = {
                mPointLights[0].Pos,
                mPointLights[1].Pos,
                mSpotLight.Pos,
            };
            const glm::vec3 markerColors[3] = {
                mPointLights[0].Diffuse,
                mPointLights[1].Diffuse,
                mSpotLight.Diffuse,
            };
            for (int i = 0; i < 3; ++i)
            {
                if (mFlashLightMode && i >= 2)
                {
                    continue;
                }
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
            Uniforms::SetVec3(*mProgram.get(), "viewPos", mCamera.Pos);

            // --- Light 활성 플래그 — 셰이더가 enabled==0 슬롯의 Calc* 호출을 건너뜀.
            //     ImGui 체크박스로 토글된 광원은 화면에 기여 안 함.
            Uniforms::SetInt(*mProgram.get(), "dirLightEnabled", mDirLightEnabled ? 1 : 0);
            Uniforms::SetInt(*mProgram.get(), "spotLightEnabled", mSpotLightEnabled ? 1 : 0);
            for (int i = 0; i < 2; ++i)
            {
                const std::string name = "pointLightsEnabled[" + std::to_string(i) + "]";
                Uniforms::SetInt(*mProgram.get(), name.c_str(), mPointLightsEnabled[i] ? 1 : 0);
            }
            if (mFlashLightMode)
            {
                mSpotLight.Pos = mCamera.Pos;
                mSpotLight.Direction = mCamera.GetFront();
            }
            Uniforms::SetSpotLight(*mProgram.get(), "spotLight", mSpotLight);
            // --- DirLight 1개 (평행광 / 거리감쇠 없음) ---
            Uniforms::SetDirLight(*mProgram, "dirLight", mDirLight);

            // --- PointLight 2개 (배열, 거리감쇠는 helper 가 mDistance 로 내부 도출) ---
            for (int i = 0; i < 2; ++i)
            {
                const std::string base = "pointLights[" + std::to_string(i) + "]";
                Uniforms::SetPointLight(*mProgram, base.c_str(), mPointLights[i]);
            }

            // --- SpotLight 1개 (점광원 + 콘 cutoff — degree->cosine 변환은 helper 내부) ---
            {
                Uniforms::SetSpotLight(*mProgram, "spotLight", mSpotLight);
                auto modelTransform = glm::mat4(1.0f);
                auto transform = projMat * viewMat * modelTransform;
                Uniforms::SetMat4(*mProgram.get(), "transformMat", transform);
                Uniforms::SetMat4(*mProgram.get(), "modelTransformMat", modelTransform);
            }

            {
                auto modelTransform =
                    glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 1.0f, 10.0f));
                auto transform = projMat * viewMat * modelTransform;
                Uniforms::SetMat4(*mProgram, "transformMat", transform);
                Uniforms::SetMat4(*mProgram, "modelTransformMat", modelTransform);
                auto planeMaterial = mRM->FindMaterial(STR_MATERIAL_PLANE);
                planeMaterial->Apply();
                mBox->Draw();
            }

            {
                auto modelTransform =
                    glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.75f, -4.0f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(1.5f, 1.5f, 1.5f));
                auto transform = projMat * viewMat * modelTransform;
                Uniforms::SetMat4(*mProgram, "transformMat", transform);
                Uniforms::SetMat4(*mProgram, "modelTransformMat", modelTransform);
                auto box1Material = mRM->FindMaterial(STR_MATERIAL_BOX1);
                box1Material->Apply();
                mBox->Draw();
            }

            {
                auto modelTransform =
                    glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.749f, 2.0f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(1.5f, 1.5f, 1.5f));
                auto transform = projMat * viewMat * modelTransform;
                Uniforms::SetMat4(*mProgram, "transformMat", transform);
                Uniforms::SetMat4(*mProgram, "modelTransformMat", modelTransform);
                auto box2Material = mRM->FindMaterial(STR_MATERIAL_BOX2);
                box2Material->Apply();
                mBox->Draw();
            }
        }

        // glPointSize(50.0f);
        // glDrawArrays(GL_POINTS, 0, 1);
    }

    bool Context::Init()
    {
        // === Light Casters 초기화 (사용자 제공 reference 값) ===
        // 거리감쇠 c1=0.09 / c2=0.032 -> learnopengl 표 distance≈50 행에 대응하므로 mDistance=50 로 설정.
        // (project 의 GetAttenuationCoeff 가 distance->(Kc,Kl,Kq) 변환을 담당)

        mDirLight = {
            .Direction = glm::vec3(0.0f, -1.0f, 0.0f),
            .Ambient = glm::vec3(1.0f, 1.0f, 1.0f),
            .Diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
            .Specular = glm::vec3(1.0f, 1.0f, 1.0f)};

        mPointLights[0] = {.Pos = glm::vec3(1.2f, 1.0f, 1.0f),
                           .Distance = 50.0f,
                           .Ambient = glm::vec3(0.05f, 0.05f, 0.05f),
                           .Diffuse = glm::vec3(0.8f, 0.4f, 0.2f),
                           .Specular = glm::vec3(1.0f, 1.0f, 1.0f)};

        mPointLights[1] = {.Pos = glm::vec3(-1.2f, 1.0f, -1.0f),
                           .Distance = 50.0f,
                           .Ambient = glm::vec3(0.05f, 0.05f, 0.05f),
                           .Diffuse = glm::vec3(0.2f, 0.4f, 0.8f),
                           .Specular = glm::vec3(1.0f, 1.0f, 1.0f)};

        mSpotLight = {.Pos = glm::vec3(1.0f, 4.0f, 4.0f),
                      .Direction = glm::vec3(0.0f, -1.0f, 0.0f),
                      .CutoffAngleDeg = 5.0f,
                      .OuterCutoffAngleDeg = 120.0f,
                      .Distance = 128.0f,
                      .Ambient = glm::vec3(0.0f, 0.0f, 0.0f),
                      .Diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
                      .Specular = glm::vec3(1.0f, 1.0f, 1.0f)};

        mCamera = {
            .Pos = glm::vec3(0.0f, 2.5f, 8.0f),
            .EulerPitch = -20.f,
        };

        mProgram = Program::CreateWithVSFS("./resources/shader/lighting.vs", "./resources/shader/lighting.fs");
        if (!mProgram)
            return false;

        mSimpleProgram = Program::CreateWithVSFS("./resources/shader/simple.vs", "./resources/shader/simple.fs");
        if (!mSimpleProgram)
            return false;

        spdlog::info("program id: {}", mProgram->GetProgramAddr());
        glClearColor(0.0, 0.1f, 0.2f, 0.0f);

        mRM = ResourceRegistry::Create();

        auto imageDarkGary = Image::Create(STR_IMAGE_DARK_GRAY, 4, 4, 4);
        imageDarkGary->SetSingleColorImage(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));

        auto textureDarkGray = mRM->CreateTexture(
            STR_TEXTURE_DARK_GRAY, imageDarkGary.get());

        auto imageGray = Image::Create(STR_IMAGE_GRAY, 4, 4, 4);
        imageGray->SetSingleColorImage(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));

        auto textureGray = mRM->CreateTexture(
            STR_TEXTURE_GRAY, imageGray.get());

        auto imageMarble = Image::Load(STR_IMAGE_MARBLE, "./resources/texture/marble.jpg");
        auto textureMarble = mRM->CreateTexture(
            STR_TEXTURE_MARBLE, imageMarble.get());

        // plane — diffuse: 그레이, specular: marble, shininess 128.
        // CreateMaterial 은 빈 Material 만 — 텍스처는 위에서 만든 것을 여기서 명시 주입.
        auto planeMaterial = mRM->CreateMaterial(STR_MATERIAL_PLANE);
        planeMaterial->SetResolvedTextures(textureMarble, 0, textureGray, 1);
        planeMaterial->SetShininess(128.0f);
        planeMaterial->SetProgram(mProgram.get());

        // box1 — diffuse: container.jpg, specular: 다크 그레이, shininess 16.
        auto imageBox1Diffuse = Image::Load(STR_IMAGE_BOX1_DIFFUSE, "./resources/texture/container.jpg");
        auto textureBox1Diffuse = mRM->CreateTexture(STR_TEXTURE_BOX1_DIFFUSE, imageBox1Diffuse.get());
        auto box1Material = mRM->CreateMaterial(STR_MATERIAL_BOX1);
        box1Material->SetResolvedTextures(textureBox1Diffuse, 0, textureDarkGray, 1);
        box1Material->SetShininess(16.0f);
        box1Material->SetProgram(mProgram.get());

        // box2 — diffuse: container2.png, specular: container2_specular.png, shininess 64.
        auto imageBox2Diffuse = Image::Load(STR_IMAGE_BOX2_DIFFUSE, "./resources/texture/container2.png");
        auto textureBox2Diffuse = mRM->CreateTexture(STR_TEXTURE_BOX2_DIFFUSE, imageBox2Diffuse.get());
        auto imageBox2Specular = Image::Load(STR_IMAGE_BOX2_SPECULAR, "./resources/texture/container2_specular.png");
        auto textureBox2Specular = mRM->CreateTexture(STR_TEXTURE_BOX2_SPECULAR, imageBox2Specular.get());
        auto box2Material = mRM->CreateMaterial(STR_MATERIAL_BOX2);
        box2Material->SetResolvedTextures(textureBox2Diffuse, 0, textureBox2Specular, 1);
        box2Material->SetShininess(64.0f);
        box2Material->SetProgram(mProgram.get());
        /*
        순서
            1. VAO
            2. VBO
            3. Vertex Attribute Setting
        */

        // 큐브 메시 1개 — VAO/VBO/EBO 는 Mesh 가 캡슐화. unbind 도 Mesh::Init 내부에서 처리됨.
        // (직전: 여기서 glBindVertexArray(0) / glBindBuffer 3종을 직접 호출했지만 Mesh 캡슐화 후 redundant.
        //  마찬가지로 unit 0/1 에 white 텍스처를 바인딩하고 lighting.fs 의 tex0/tex1 sampler 에 SetInt 까지 했지만,
        //  multi-light 마이그레이션 후 두 sampler 가 셰이더에서 제거되었고 simple.fs 는 sampler 자체가 없음 ->
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
