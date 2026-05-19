#include "context.h"
#include "buffer/buffer.h"
#include "common/constants.h"
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

    namespace
    {
        constexpr float kCameraSpeed = 0.05f;   // WASD/QE 이동 속도 (프레임당 world unit)
        constexpr float kCameraRotSpeed = 0.1f; // 마우스 드래그 -> yaw/pitch 회전 배율
    }

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
        // 우클릭 드래그 중일 때만 카메라 이동 — 기존 IsCamControl 가드와 동일 거동.
        if (mMouse.IsDragging())
            mKeyboard.PollHeld(window);
    }

    void Context::Reshape(int width, int height)
    {
        mWidth = width;
        mHeight = height;
        glViewport(0, 0, mWidth, mHeight);

        mCamera.SetAspect((float)width, (float)height); // height==0 가드 내장

        mFramebuffer = Framebuffer::Create(Texture::Create(width, height, GL_RGBA));
    }

    void Context::MouseMove(double x, double y)
    {
        mMouse.HandleMove(x, y);
    }

    void Context::MouseButton(int button, int action, double x, double y)
    {
        mMouse.HandleButton(button, action, x, y);
    }

    void Context::Render()
    {
        // ... 기존 ImGui 코드 ...
        if (ImGui::Begin(Const::LBL_UI_WINDOW))
        {
            if (ImGui::CollapsingHeader(Const::LBL_DIRLIGHT))
            {
                ImGui::Checkbox(Const::LBL_DIR_ENABLED, &mDirLightEnabled);
                {
                    Transform t = mScene.At(SceneNodeId::DirLight).Local();
                    if (ImGui::DragFloat3(Const::LBL_DIR_DIRECTION, glm::value_ptr(t.EulerRot), 0.5f))
                        mScene.At(SceneNodeId::DirLight).SetLocal(t);
                }
                ImGui::ColorEdit3(Const::LBL_DIR_AMBIENT, glm::value_ptr(mDirLight.Ambient));
                ImGui::ColorEdit3(Const::LBL_DIR_DIFFUSE, glm::value_ptr(mDirLight.Diffuse));
                ImGui::ColorEdit3(Const::LBL_DIR_SPECULAR, glm::value_ptr(mDirLight.Specular));
            }

            for (int i = 0; i < 2; ++i)
            {
                ImGui::PushID(i); // 같은 라벨 충돌 방지
                std::string header = Const::LBL_POINTLIGHT_PREFIX + std::to_string(i) + Const::STR_INDEX_CLOSE;
                if (ImGui::CollapsingHeader(header.c_str()))
                {
                    ImGui::Checkbox(Const::LBL_P_ENABLED, &mPointLightsEnabled[i]);
                    {
                        const SceneNodeId pid = (i == 0) ? SceneNodeId::PointLight0 : SceneNodeId::PointLight1;
                        Transform t = mScene.At(pid).Local();
                        if (ImGui::DragFloat3(Const::LBL_P_POSITION, glm::value_ptr(t.Translate), 0.01f))
                            mScene.At(pid).SetLocal(t);
                    }
                    ImGui::DragFloat(Const::LBL_P_DISTANCE, &mPointLights[i].Distance, 0.5f, 1.0f, 3250.0f);
                    ImGui::ColorEdit3(Const::LBL_P_AMBIENT, glm::value_ptr(mPointLights[i].Ambient));
                    ImGui::ColorEdit3(Const::LBL_P_DIFFUSE, glm::value_ptr(mPointLights[i].Diffuse));
                    ImGui::ColorEdit3(Const::LBL_P_SPECULAR, glm::value_ptr(mPointLights[i].Specular));
                }
                ImGui::PopID();
            }

            if (ImGui::CollapsingHeader(Const::LBL_SPOTLIGHT, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox(Const::LBL_S_ENABLED, &mSpotLightEnabled);
                {
                    Transform t = mScene.At(SceneNodeId::SpotLight).Local();
                    bool spotChanged = false;
                    spotChanged |= ImGui::DragFloat3(Const::LBL_S_POSITION, glm::value_ptr(t.Translate), 0.01f);
                    spotChanged |= ImGui::DragFloat3(Const::LBL_S_DIRECTION, glm::value_ptr(t.EulerRot), 0.5f);
                    if (spotChanged)
                        mScene.At(SceneNodeId::SpotLight).SetLocal(t);
                }
                ImGui::DragFloat(Const::LBL_S_CUTOFF, &mSpotLight.CutoffAngleDeg, 0.1f, 0.0f, 89.0f);
                ImGui::DragFloat(Const::LBL_S_OUTER_CUTOFF, &mSpotLight.OuterCutoffAngleDeg, 0.1f, 0.0f, 90.0f);
                ImGui::DragFloat(Const::LBL_S_DISTANCE, &mSpotLight.Distance, 0.5f, 1.0f, 3250.0f);
                ImGui::ColorEdit3(Const::LBL_S_AMBIENT, glm::value_ptr(mSpotLight.Ambient));
                ImGui::ColorEdit3(Const::LBL_S_DIFFUSE, glm::value_ptr(mSpotLight.Diffuse));
                ImGui::ColorEdit3(Const::LBL_S_SPECULAR, glm::value_ptr(mSpotLight.Specular));
                ImGui::Checkbox(Const::LBL_FLASH_LIGHT, &mFlashLightMode);
            }

            ImGui::Checkbox(Const::LBL_ANIMATION, &mAnimation);

            // 함수 하나가 UI component 하나에 대응
            // 리턴 값이 true인 경우 해당 UI가 조작되었음을 의미
            // UI 조작 이벤트에 대한 액션 로직을 if으로 작성할 수 있음
            if (ImGui::ColorEdit4(Const::LBL_CLEAR_COLOR, glm::value_ptr(mClearColor)))
            {
                glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
            }
            ImGui::Separator();
            ImGui::DragFloat(Const::LBL_POSTPROCESS_GAMMA, &mGamma, 0.01f, 0.0f, 2.0f);
            {
                Transform camT = mScene.At(SceneNodeId::Camera).Local();
                bool camChanged = false;
                camChanged |= ImGui::DragFloat3(Const::LBL_CAMERA_POS, glm::value_ptr(camT.Translate), 0.01f);
                camChanged |= ImGui::DragFloat(Const::LBL_CAMERA_YAW, &camT.EulerRot.y, 0.5f);
                camChanged |= ImGui::DragFloat(Const::LBL_CAMERA_PITCH, &camT.EulerRot.x, 0.5f, -89.0f, 89.0f);
                if (camChanged)
                    mScene.At(SceneNodeId::Camera).SetLocal(camT);
            }
            ImGui::Separator();
            if (ImGui::Button(Const::LBL_RESET_CAMERA))
            {
                mScene.At(SceneNodeId::Camera).SetLocal(Transform{.Translate = {0.0f, 0.0f, 3.0f}});
            }

            ImGui::Combo(Const::LBL_DEPTH_FUNC, &mDepthFuncIndex,
                         Const::DEPTH_FUNC_LABELS, IM_ARRAYSIZE(Const::DEPTH_FUNC_LABELS));
        }
        ImGui::End();

        mFramebuffer->Bind();

        // 1. ★ 매 프레임 stencil buffer 를 0 으로 리셋 ★
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST); // 깊이 테스트 사용하기
        glEnable(GL_CULL_FACE);  // face culling 활성화
        glFrontFace(GL_CW);
        glCullFace(GL_BACK);
        glCullFace(GL_FRONT);
        // glDisable(GL_DEPTH_TEST); // 깊이 테스트 사용하지 않기.
        // glDepthMask(GL_FALSE); // depth buffer의 업데이트 막기
        glClearDepth(1.0f); // depth buffer의 초기값 설정하기

        // 비교 연산자 룩업 — 순서는 ImGui Combo 의 Const::DEPTH_FUNC_LABELS[] 와 1:1 동일해야 함.

        // ImGui Combo 가 고른 인덱스로 depth test 비교 연산자 적용.
        {
            /* perspective projection을 적용하면 깊이값을 0~1 사이로 정규화하면서 w값으로 나누는 과정을 거침
            정규화된 z값은 1/z 꼴의 함수 형태로 분포가 나타남

            깊이 값의 왜곡 멀리 있는 픽셀 간에 z값의 오차가 크지 않아서 문제가 발생할 수 있음 z-fighting
            예방법 :
                면과 면을 너무 붙어있게 하지 않을 것
                near의 값을 너무 작게 하지 말것
                좀더 정확한 depth buffer를 설정하여 사용할 것
                    예를 들어 DepthBUffer는 24bit, 32bit ... 등등이 있다.
                    그리고 32bit이 좀더 정교하지 않겠느냐~ 라는것.
            */
            // mCamera.FarPlane = 10.f;
            // mCamera.NearPlane = 0.5f;
        }
        glDepthFunc(Const::DEPTH_FUNC[mDepthFuncIndex]);

        // 카메라 노드의 월드 변환 역행렬 = view 행렬. proj 는 렌즈(mCamera).
        auto viewMat = glm::inverse(mScene.World(SceneNodeId::Camera));
        auto projMat = mCamera.GetProjMatrix(); // mAspect 멤버 사용 (Reshape 에서 갱신)

        // 2. Use Program — 광원 *위치 표시 큐브* 들 (DirLight 는 방향만 가지므로 표시 안 함)
        mSimpleProgram->Use();
        {
            // 두 점광원 + 스포트라이트 = 총 3개 마커 큐브. 각자 자기 diffuse 색으로 출력.
            // 마커 = 라이트 노드 자체 — 노드 World() 에 직접 그림 (per-frame 위치 복사 없음).
            const glm::vec3 markerColors[3] = {
                mPointLights[0].Diffuse,
                mPointLights[1].Diffuse,
                mSpotLight.Diffuse,
            };
            const SceneNodeId markerIds[3] = {
                SceneNodeId::PointLight0, SceneNodeId::PointLight1, SceneNodeId::SpotLight};
            for (int i = 0; i < 3; ++i)
            {
                if (mFlashLightMode && i >= 2)
                    continue;
                Uniforms::SetVec4(*mSimpleProgram.get(), Const::UNI_BASE_COLOR, glm::vec4(markerColors[i], 1.0f));
                Uniforms::SetMat4(*mSimpleProgram.get(), Const::UNI_TRANSFORM_MAT,
                                  projMat * viewMat * mScene.World(markerIds[i]));
                mBox->Draw();
            }
        }

        mProgram->Use();
        {
            Uniforms::SetVec3(*mProgram.get(), Const::UNI_VIEW_POS,
                              glm::vec3(mScene.World(SceneNodeId::Camera)[3]));

            // --- Light 활성 플래그 — 셰이더가 enabled==0 슬롯의 Calc* 호출을 건너뜀.
            //     ImGui 체크박스로 토글된 광원은 화면에 기여 안 함.
            Uniforms::SetInt(*mProgram.get(), Const::UNI_DIR_LIGHT_ENABLED, mDirLightEnabled ? 1 : 0);
            Uniforms::SetInt(*mProgram.get(), Const::UNI_SPOT_LIGHT_ENABLED, mSpotLightEnabled ? 1 : 0);
            for (int i = 0; i < 2; ++i)
            {
                const std::string name = Const::UNI_POINT_LIGHTS_ENABLED_PREFIX + std::to_string(i) + Const::STR_INDEX_CLOSE;
                Uniforms::SetInt(*mProgram.get(), name.c_str(), mPointLightsEnabled[i] ? 1 : 0);
            }
            if (mFlashLightMode)
            {
                // 스포트 노드가 카메라 노드를 추종 — 마커 큐브 크기(Scale 0.1) 는 보존.
                Transform spotT = mScene.At(SceneNodeId::Camera).Local();
                spotT.Scale = glm::vec3(0.1f, 0.1f, 0.1f);
                mScene.At(SceneNodeId::SpotLight).SetLocal(spotT);
            }
            Uniforms::SetSpotLight(*mProgram.get(), Const::UNI_SPOT_LIGHT, mSpotLight,
                                   glm::vec3(mScene.World(SceneNodeId::SpotLight)[3]),
                                   mScene.WorldForward(SceneNodeId::SpotLight));
            // --- DirLight 1개 (평행광 / 거리감쇠 없음) ---
            Uniforms::SetDirLight(*mProgram, Const::UNI_DIR_LIGHT, mDirLight,
                                  mScene.WorldForward(SceneNodeId::DirLight));

            // --- PointLight 2개 (배열, 거리감쇠는 helper 가 mDistance 로 내부 도출) ---
            const SceneNodeId pointLightIds[2] = {
                SceneNodeId::PointLight0, SceneNodeId::PointLight1};
            for (int i = 0; i < 2; ++i)
            {
                const std::string base = Const::UNI_POINT_LIGHTS_PREFIX + std::to_string(i) + Const::STR_INDEX_CLOSE;
                Uniforms::SetPointLight(*mProgram, base.c_str(), mPointLights[i],
                                        glm::vec3(mScene.World(pointLightIds[i])[3]));
            }

            // --- SpotLight uniform 은 위에서 전송됨 — 본 블록은 transform uniform 만 ---
            {
                auto modelTransform = glm::mat4(1.0f);
                auto transform = projMat * viewMat * modelTransform;
                Uniforms::SetMat4(*mProgram.get(), Const::UNI_TRANSFORM_MAT, transform);
                Uniforms::SetMat4(*mProgram.get(), Const::UNI_MODEL_TRANSFORM_MAT, modelTransform);
            }

            {
                auto modelTransform = mScene.World(SceneNodeId::Plane);
                auto transform = projMat * viewMat * modelTransform;
                Uniforms::SetMat4(*mProgram, Const::UNI_TRANSFORM_MAT, transform);
                Uniforms::SetMat4(*mProgram, Const::UNI_MODEL_TRANSFORM_MAT, modelTransform);
                auto planeMaterial = mRM->FindMaterial(Const::STR_MATERIAL_PLANE);
                planeMaterial->Apply();
                mBox->Draw();
            }

            {
                auto modelTransform = mScene.World(SceneNodeId::Box1);
                auto transform = projMat * viewMat * modelTransform;
                Uniforms::SetMat4(*mProgram, Const::UNI_TRANSFORM_MAT, transform);
                Uniforms::SetMat4(*mProgram, Const::UNI_MODEL_TRANSFORM_MAT, modelTransform);
                auto box1Material = mRM->FindMaterial(Const::STR_MATERIAL_BOX1);
                box1Material->Apply();
                mBox->Draw();
            }

            // 2. 본체 그릴 때 stencil=1 도장
            glEnable(GL_STENCIL_TEST);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(0xFF);

            // 3. 본체 렌더 (lighting program)
            {
                auto modelTransform = mScene.World(SceneNodeId::Box2);
                auto transform = projMat * viewMat * modelTransform;
                Uniforms::SetMat4(*mProgram, Const::UNI_TRANSFORM_MAT, transform);
                Uniforms::SetMat4(*mProgram, Const::UNI_MODEL_TRANSFORM_MAT, modelTransform);

                auto box2Material = mRM->FindMaterial(Const::STR_MATERIAL_BOX2);
                box2Material->Apply();
                mBox->Draw();
            }

            // 4. outline 단계 — stencil!=1 만, stencil 쓰기 잠금,
            //      depth off
            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
            glStencilMask(0x00);
            glDisable(GL_DEPTH_TEST);

            // 5. 키운 박스 + outline 셰이더 렌더
            mSimpleProgram->Use();
            {
                auto transform = projMat * viewMat * mScene.World(SceneNodeId::Outline);
                Uniforms::SetVec4(*mSimpleProgram.get(), Const::UNI_BASE_COLOR, glm::vec4(1.0f, 1.0f, 0.5f, 1.0f));
                Uniforms::SetMat4(*mSimpleProgram.get(), Const::UNI_TRANSFORM_MAT, transform);

                mBox->Draw();
            }

            // 6. 복원
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_STENCIL_TEST);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(0xFF);
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        mTextureProgram->Use();
        {
            // 카메라 앞의 유리창을 먼저 그리는 경우 이 유리창이 depth buffer 값을 갱신
            // 뒤의 유리창은 depth test를 통과하지 못하고 그려지지 않음
            auto textureWindow = mRM->FindTexture(Const::STR_TEXTURE_WINDOW);
            if (textureWindow)
            {
                glActiveTexture(GL_TEXTURE0 + 0);
                textureWindow->Bind();
                Uniforms::SetInt(*mTextureProgram, Const::UNI_TEX, 0);
            }

            auto modelTransform = mScene.World(SceneNodeId::Window0);
            auto transform = projMat * viewMat * modelTransform;
            Uniforms::SetMat4(*mTextureProgram, Const::UNI_TRANSFORM_MAT, transform);
            mPlane->Draw();

            modelTransform = mScene.World(SceneNodeId::Window1);
            transform = projMat * viewMat * modelTransform;
            Uniforms::SetMat4(*mTextureProgram, Const::UNI_TRANSFORM_MAT, transform);
            mPlane->Draw();

            modelTransform = mScene.World(SceneNodeId::Window2);
            transform = projMat * viewMat * modelTransform;
            Uniforms::SetMat4(*mTextureProgram, Const::UNI_TRANSFORM_MAT, transform);
            mPlane->Draw();
        }

        Framebuffer::BindToDefault();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // 텍스쳐에 넣는거.
        // mTextureProgram->Use();
        // Uniforms::SetMat4(*mTextureProgram, Const::UNI_TRANSFORM_MAT, glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 1.0f)));

        // mFramebuffer
        //     ->GetColorAttachment()
        //     ->Bind();
        // Uniforms::SetInt(*mTextureProgram, Const::UNI_FRAMETEXTURE, 0);
        // mPlane->Draw();

        Framebuffer::BindToDefault();

        mPostProgram->Use();
        Uniforms::SetMat4(*mPostProgram, Const::UNI_TRANSFORM_MAT, glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 1.0f)));
        Uniforms::SetFloat(*mPostProgram, Const::UNI_POSTPROCESS_GAMMA, mGamma);
        mFramebuffer
            ->GetColorAttachment() // TexturePtr m_colorAttachment;
            ->Bind();              // glBindTexture(GL_TEXTURE_2D, mTextureID);
        Uniforms::SetInt(*mPostProgram, Const::UNI_POSTPROCESS_FRAMETEXTURE, 0);
        mPlane->Draw();
    }

    bool Context::Init()
    {
        mDirLight = {
            .Ambient = glm::vec3(1.0f, 1.0f, 1.0f),
            .Diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
            .Specular = glm::vec3(1.0f, 1.0f, 1.0f)};

        mPointLights[0] = {.Distance = 50.0f,
                           .Ambient = glm::vec3(0.05f, 0.05f, 0.05f),
                           .Diffuse = glm::vec3(0.8f, 0.4f, 0.2f),
                           .Specular = glm::vec3(1.0f, 1.0f, 1.0f)};

        mPointLights[1] = {.Distance = 50.0f,
                           .Ambient = glm::vec3(0.05f, 0.05f, 0.05f),
                           .Diffuse = glm::vec3(0.2f, 0.4f, 0.8f),
                           .Specular = glm::vec3(1.0f, 1.0f, 1.0f)};

        mSpotLight = {.CutoffAngleDeg = 5.0f,
                      .OuterCutoffAngleDeg = 120.0f,
                      .Distance = 128.0f,
                      .Ambient = glm::vec3(0.0f, 0.0f, 0.0f),
                      .Diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
                      .Specular = glm::vec3(1.0f, 1.0f, 1.0f)};

        // === Input 바인딩 ===
        // 1단 — 물리 키 -> 논리 액션 (InputMap 계층).
        mKeyboard.BindKey(GLFW_KEY_W, GameAction::MoveForward);
        mKeyboard.BindKey(GLFW_KEY_S, GameAction::MoveBackward);
        mKeyboard.BindKey(GLFW_KEY_D, GameAction::MoveRight);
        mKeyboard.BindKey(GLFW_KEY_A, GameAction::MoveLeft);
        mKeyboard.BindKey(GLFW_KEY_E, GameAction::MoveUp);
        mKeyboard.BindKey(GLFW_KEY_Q, GameAction::MoveDown);

        // 2단 — 논리 액션 -> 핸들러 (연속 이동). 카메라 노드를 증분 갱신.
        // front = 카메라 노드 월드 전방, right/up 은 수평 strafe 위해 world-up 과 외적.
        mKeyboard.BindHeldHandler(GameAction::MoveForward, [this]
                                  { mScene.At(SceneNodeId::Camera)
                                        .TranslateBy(kCameraSpeed * mScene.WorldForward(SceneNodeId::Camera)); });
        mKeyboard.BindHeldHandler(GameAction::MoveBackward, [this]
                                  { mScene.At(SceneNodeId::Camera)
                                        .TranslateBy(-kCameraSpeed * mScene.WorldForward(SceneNodeId::Camera)); });
        mKeyboard.BindHeldHandler(GameAction::MoveRight, [this]
                                  {
            const glm::vec3 front = mScene.WorldForward(SceneNodeId::Camera);
            const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -front));
            mScene.At(SceneNodeId::Camera).TranslateBy(kCameraSpeed * right); });
        mKeyboard.BindHeldHandler(GameAction::MoveLeft, [this]
                                  {
            const glm::vec3 front = mScene.WorldForward(SceneNodeId::Camera);
            const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -front));
            mScene.At(SceneNodeId::Camera).TranslateBy(-kCameraSpeed * right); });
        mKeyboard.BindHeldHandler(GameAction::MoveUp, [this]
                                  {
            const glm::vec3 front = mScene.WorldForward(SceneNodeId::Camera);
            const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -front));
            const glm::vec3 up = glm::normalize(glm::cross(-front, right));
            mScene.At(SceneNodeId::Camera).TranslateBy(kCameraSpeed * up); });
        mKeyboard.BindHeldHandler(GameAction::MoveDown, [this]
                                  {
            const glm::vec3 front = mScene.WorldForward(SceneNodeId::Camera);
            const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -front));
            const glm::vec3 up = glm::normalize(glm::cross(-front, right));
            mScene.At(SceneNodeId::Camera).TranslateBy(-kCameraSpeed * up); });

        // 마우스 우클릭 드래그 -> 카메라 노드 EulerRot (yaw=.y / pitch=.x) 갱신.
        mMouse.BindLookHandler([this](double dx, double dy)
                               {
            glm::vec3 rot = mScene.At(SceneNodeId::Camera).Local().EulerRot;
            rot.y -= static_cast<float>(dx) * kCameraRotSpeed; // yaw
            rot.x -= static_cast<float>(dy) * kCameraRotSpeed; // pitch
            if (rot.y < 0.0f)
                rot.y += 360.0f;
            if (rot.y > 360.0f)
                rot.y -= 360.0f;
            if (rot.x > 89.0f)
                rot.x = 89.0f;
            if (rot.x < -89.0f)
                rot.x = -89.0f;
            mScene.At(SceneNodeId::Camera).SetEulerRot(rot); });

        mProgram = Program::CreateWithVSFS(Const::PATH_SHADER_LIGHTING_VS, Const::PATH_SHADER_LIGHTING_FS);
        if (!mProgram)
            return false;

        mSimpleProgram = Program::CreateWithVSFS(Const::PATH_SHADER_SIMPLE_VS, Const::PATH_SHADER_SIMPLE_FS);
        if (!mSimpleProgram)
            return false;

        mTextureProgram = Program::CreateWithVSFS(Const::PATH_SHADER_TEXTURE_VS, Const::PATH_SHADER_TEXTURE_FS);
        if (!mTextureProgram)
            return false;

        // mPostProgram = Program::CreateWithVSFS(Const::PATH_SHADER_TEXTURE_VS, Const::PATH_SHADER_POST_INVERT_FS);
        // mPostProgram = Program::CreateWithVSFS(Const::PATH_SHADER_TEXTURE_VS, Const::PATH_SHADER_POSTPROCESS_GAMMA_FS);
        // mPostProgram = Program::CreateWithVSFS(Const::PATH_SHADER_TEXTURE_VS, Const::PATH_SHADER_POSTPROCESS_SHARPEN_FS);
        // mPostProgram = Program::CreateWithVSFS(Const::PATH_SHADER_TEXTURE_VS, Const::PATH_SHADER_POSTPROCESS_SOBLE_FS);
        mPostProgram = Program::CreateWithVSFS(Const::PATH_SHADER_TEXTURE_VS, Const::PATH_SHADER_POSTPROCESS_BLUR_FS);
        if (!mPostProgram)
            return false;

        spdlog::info("program id: {}", mProgram->GetProgramAddr());
        glClearColor(0.0, 0.1f, 0.2f, 0.0f);

        mRM = ResourceRegistry::Create();

        auto imageDarkGary = Image::Create(Const::STR_IMAGE_DARK_GRAY, 4, 4, 4);
        imageDarkGary->SetSingleColorImage(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
        auto imageGray = Image::Create(Const::STR_IMAGE_GRAY, 4, 4, 4);
        imageGray->SetSingleColorImage(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
        auto imageMarble = Image::Load(Const::STR_IMAGE_MARBLE, Const::PATH_TEX_MARBLE);

        auto textureDarkGray = mRM->CreateTexture(
            Const::STR_TEXTURE_DARK_GRAY, imageDarkGary.get());
        auto textureGray = mRM->CreateTexture(
            Const::STR_TEXTURE_GRAY, imageGray.get());
        auto textureMarble = mRM->CreateTexture(
            Const::STR_TEXTURE_MARBLE, imageMarble.get());

        // plane — diffuse: 그레이, specular: marble, shininess 128.
        // CreateMaterial 은 빈 Material 만 — 텍스처는 위에서 만든 것을 여기서 명시 주입.
        auto planeMaterial = mRM->CreateMaterial(Const::STR_MATERIAL_PLANE);
        planeMaterial->SetResolvedTextures(textureMarble, 0, textureGray, 1);
        planeMaterial->SetShininess(128.0f);
        planeMaterial->SetProgram(mProgram.get());

        // box1 — diffuse: container.jpg, specular: 다크 그레이, shininess 16.
        auto imageBox1Diffuse = Image::Load(Const::STR_IMAGE_BOX1_DIFFUSE, Const::PATH_TEX_CONTAINER);
        auto textureBox1Diffuse = mRM->CreateTexture(Const::STR_TEXTURE_BOX1_DIFFUSE, imageBox1Diffuse.get());
        auto box1Material = mRM->CreateMaterial(Const::STR_MATERIAL_BOX1);
        box1Material->SetResolvedTextures(textureBox1Diffuse, 0, textureDarkGray, 1);
        box1Material->SetShininess(16.0f);
        box1Material->SetProgram(mProgram.get());

        // box2 — diffuse: container2.png, specular: container2_specular.png, shininess 64.
        auto imageBox2Diffuse = Image::Load(Const::STR_IMAGE_BOX2_DIFFUSE, Const::PATH_TEX_CONTAINER2);
        auto textureBox2Diffuse = mRM->CreateTexture(Const::STR_TEXTURE_BOX2_DIFFUSE, imageBox2Diffuse.get());
        auto imageBox2Specular = Image::Load(Const::STR_IMAGE_BOX2_SPECULAR, Const::PATH_TEX_CONTAINER2_SPECULAR);
        auto textureBox2Specular = mRM->CreateTexture(Const::STR_TEXTURE_BOX2_SPECULAR, imageBox2Specular.get());
        auto box2Material = mRM->CreateMaterial(Const::STR_MATERIAL_BOX2);
        box2Material->SetResolvedTextures(textureBox2Diffuse, 0, textureBox2Specular, 1);
        box2Material->SetShininess(64.0f);
        box2Material->SetProgram(mProgram.get());

        // plane
        auto imageWindow = Image::Load(Const::STR_TEXTURE_WINDOW, Const::PATH_TEX_WINDOW);
        auto textureWindow = mRM->CreateTexture(Const::STR_TEXTURE_WINDOW, imageWindow.get());

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
        mPlane = Mesh::CreatePlane();

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

        mBox->GetVertexLayout()->Bind();
        mSimpleProgram->Use();

        Diagnostics::GLValidate::CheckAttribLayout(mSimpleProgram->GetProgramAddr(), "Context::Init");
        Diagnostics::GLValidate::CheckSamplerBindings(mSimpleProgram->GetProgramAddr(), "Context::Init");
        Diagnostics::GLValidate::DumpShaderInfoLogs(mSimpleProgram->GetProgramAddr(), "Context::Init");

        mPlane->GetVertexLayout()->Bind();
        mTextureProgram->Use();

        Diagnostics::GLValidate::CheckAttribLayout(mTextureProgram->GetProgramAddr(), "Context::Init");
        Diagnostics::GLValidate::CheckSamplerBindings(mTextureProgram->GetProgramAddr(), "Context::Init");
        Diagnostics::GLValidate::DumpShaderInfoLogs(mTextureProgram->GetProgramAddr(), "Context::Init");

        // === Scene Graph 구성 ===
        // 정적 노드의 로컬 변환 + 계층. 동적 노드(마커)는 Render() 가 매 프레임 갱신.
        mScene.SetLocal(SceneNodeId::Plane,
                        Transform{.Translate = {0.0f, -0.5f, 0.0f},
                                  .Scale = {10.0f, 1.0f, 10.0f}});
        mScene.SetLocal(SceneNodeId::Box1,
                        Transform{.Translate = {-1.0f, 0.75f, -4.0f},
                                  .EulerRot = {0.0f, 30.0f, 0.0f},
                                  .Scale = {1.5f, 1.5f, 1.5f}});
        mScene.SetLocal(SceneNodeId::Box2,
                        Transform{.Translate = {0.0f, 0.75f, 2.0f},
                                  .EulerRot = {0.0f, 20.0f, 0.0f},
                                  .Scale = {1.5f, 1.5f, 1.5f}});
        // Outline 은 Box2 자식 — World(Outline) = World(Box2) * scale(1.05).
        mScene.SetLocal(SceneNodeId::Outline,
                        Transform{.Scale = {1.05f, 1.05f, 1.05f}});
        // Windows 그룹은 identity, 창들은 절대 좌표를 로컬로 — 그룹 이동 시 함께 이동.
        mScene.SetLocal(SceneNodeId::Window0, Transform{.Translate = {0.0f, 0.5f, 4.0f}});
        mScene.SetLocal(SceneNodeId::Window1, Transform{.Translate = {0.2f, 0.5f, 5.0f}});
        mScene.SetLocal(SceneNodeId::Window2, Transform{.Translate = {0.4f, 0.5f, 6.0f}});
        // 카메라 — 위치 + (pitch, yaw, 0). view 행렬은 inverse(World(Camera)).
        mScene.SetLocal(SceneNodeId::Camera,
                        Transform{.Translate = {0.0f, 2.5f, 8.0f},
                                  .EulerRot = {-20.0f, 0.0f, 0.0f}});
        // 라이트 노드 — 위치(Translate) + 마커 큐브 크기(Scale 0.1). 방향은 EulerRot.
        mScene.SetLocal(SceneNodeId::DirLight,
                        Transform{.EulerRot = {-90.0f, 0.0f, 0.0f}}); // forward = (0,-1,0)
        mScene.SetLocal(SceneNodeId::PointLight0,
                        Transform{.Translate = {1.2f, 1.0f, 1.0f}, .Scale = {0.1f, 0.1f, 0.1f}});
        mScene.SetLocal(SceneNodeId::PointLight1,
                        Transform{.Translate = {-1.2f, 1.0f, -1.0f}, .Scale = {0.1f, 0.1f, 0.1f}});
        mScene.SetLocal(SceneNodeId::SpotLight,
                        Transform{.Translate = {1.0f, 4.0f, 4.0f},
                                  .EulerRot = {-90.0f, 0.0f, 0.0f}, // forward = (0,-1,0)
                                  .Scale = {0.1f, 0.1f, 0.1f}});

        mScene.Attach(SceneNodeId::Plane, SceneNodeId::Root);
        mScene.Attach(SceneNodeId::Box1, SceneNodeId::Root);
        mScene.Attach(SceneNodeId::Box2, SceneNodeId::Root);
        mScene.Attach(SceneNodeId::Outline, SceneNodeId::Box2);
        mScene.Attach(SceneNodeId::Windows, SceneNodeId::Root);
        mScene.Attach(SceneNodeId::Window0, SceneNodeId::Windows);
        mScene.Attach(SceneNodeId::Window1, SceneNodeId::Windows);
        mScene.Attach(SceneNodeId::Window2, SceneNodeId::Windows);
        mScene.Attach(SceneNodeId::Camera, SceneNodeId::Root);
        mScene.Attach(SceneNodeId::DirLight, SceneNodeId::Root);
        mScene.Attach(SceneNodeId::PointLight0, SceneNodeId::Root);
        mScene.Attach(SceneNodeId::PointLight1, SceneNodeId::Root);
        mScene.Attach(SceneNodeId::SpotLight, SceneNodeId::Root);

        return true;
    }
}
