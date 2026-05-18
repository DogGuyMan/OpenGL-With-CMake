#ifndef __SJH_CONTEXT_H__
#define __SJH_CONTEXT_H__

#include "buffer/buffer.h"
#include "common/common.h"
#include "layout/vertex_layout.h"
#include "material/material.h"
#include "object/camera.h"
#include "object/light.h"
#include "object/mesh.h"
#include "object/model.h"
#include "program/program.h"
#include "resource_registry/resource_registry.h"
#include "shader/shader.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
namespace SJH
{
    CLASS_PTR(Context)

    /**
     * @brief 한 씬(scene)의 OpenGL 자원과 매 프레임 draw call 시퀀스를 캡슐화하는 클래스.
     *
     * @par 책임 분담 — Context 가 담당하는 것 (Scene-specific, 고변동)
     *  - VBO/EBO 생성 (@ref Buffer)              — @c GL_ARRAY_BUFFER / @c GL_ELEMENT_ARRAY_BUFFER
     *  - VAO + Vertex Attribute (@ref VertexLayout) — @c glGenVertexArrays + @c glVertexAttribPointer
     *  - 셰이더/프로그램 (@ref Shader / @ref Program)
     *  - 텍스처 로드 + 바인딩 + uniform 설정 (@ref ResourceRegistry)
     *  - 카메라 상태 보관 + 입력 → 카메라 갱신 위임 (@ref Camera)
     *  - 매 프레임 draw call 시퀀스 (@ref Render)
     *  - 위 객체들의 자동 파괴 (@c UPtr 소멸 시점)
     *
     * @par Context 가 담당하지 *않는* 것 — 앱 전반(저변동, 보일러플레이트)
     *  - GLFW @c init/terminate, OpenGL 컨텍스트 생성, glad 함수 로딩
     *  - 키/마우스 입력 콜백 등록 (콜백 자체는 @c app/main.cpp 가 GLFW 에 등록 → @ref ProcessInput / @ref MouseMove / @ref MouseButton 으로 위임)
     *  - 프레임버퍼 리사이즈 콜백 (콜백은 main 이 받고 @ref Reshape 위임)
     *  - @c glfwSwapBuffers / @c glfwPollEvents 메인 루프
     *  - 위 항목은 @c app/main.cpp 의 라이프 사이클 영역.
     */
    class Context
    {
    public:
        /**
         * @brief Context 인스턴스 생성 + 내부 GL 자원 초기화 (셰이더 컴파일/링크/VAO/VBO/EBO/텍스처).
         * @return 초기화 성공 시 @c ContextUPtr, 셰이더/프로그램/이미지 로드 실패 시 @c nullptr.
         * @pre 호출 전 OpenGL 컨텍스트가 활성화되어 있고 glad 가 로드된 상태여야 함.
         */
        static ContextUPtr Create();

        /// @brief 매 프레임 호출 — depth/color buffer 클리어 + 큐브 instance draw + view/projection uniform 갱신.
        void Render();

        /**
         * @brief 키보드 입력 폴링 → 카메라 위치 이동.
         * @param window 폴링 대상 GLFW 윈도우 (키 상태 조회용).
         * @details 매핑:
         *  - @c W / @c S — front 방향 +/-
         *  - @c A / @c D — right 방향 +/- (front × up 외적)
         *  - @c Q / @c E — up 방향 +/- (right × front 외적)
         *  @c mCamera.mIsCamControl 이 @c true 일 때만 동작.
         * @note 호출자(@c app/main.cpp 메인 루프) 가 매 프레임 직접 호출.
         */
        void ProcessInput(GLFWwindow *window);

        /**
         * @brief 프레임버퍼 리사이즈 처리 — 내부 width/height 갱신 + @c glViewport 호출.
         * @param width  새 프레임버퍼 너비 (픽셀).
         * @param height 새 프레임버퍼 높이 (픽셀).
         * @note GLFW @c framebuffer_size_callback 에서 위임받는 진입점.
         */
        void Reshape(int width, int height);

        /**
         * @brief 마우스 이동 → 카메라 yaw/pitch 회전 갱신.
         * @param x,y 현재 커서 위치 (스크린 좌표).
         * @details 이전 위치(@c mPrevMousePos)와의 델타로 회전 각도 계산:
         *  - yaw   ← @c -deltaX * 0.1
         *  - pitch ← @c -deltaY * 0.1
         *  pitch 는 @c [-89, 89] 로 클램프 (gimbal lock 회피), yaw 는 @c [0, 360) 으로 wrap.
         *  @c mCamera.mIsCamControl 이 @c true 일 때만 갱신.
         * @note GLFW @c cursor_pos_callback 에서 위임받는 진입점.
         */
        void MouseMove(double x, double y);

        /**
         * @brief 마우스 버튼 이벤트 → 카메라 회전 모드 토글.
         * @param button GLFW 버튼 코드 (@c GLFW_MOUSE_BUTTON_LEFT 등).
         * @param action @c GLFW_PRESS / @c GLFW_RELEASE.
         * @param x,y    이벤트 발생 시 커서 위치.
         * @details 좌클릭 PRESS 시 @c IsCamControl=true + @c mPrevMousePos 갱신,
         *          RELEASE 시 @c IsCamControl=false. 즉 드래그 중에만 카메라 회전.
         * @note GLFW @c mouse_button_callback 에서 위임받는 진입점.
         */
        void MouseButton(int button, int action, double x, double y);

    private:
        Context() = default;

        /// @brief 셰이더/프로그램/VAO/VBO/EBO/텍스처를 일괄 초기화. @ref Create 내부에서 한 번만 호출.
        bool Init();

        /// @brief 라이팅 셰이더 프로그램 (`lighting.vs/fs`). 큐브 본체 그릴 때 사용 — Phong + 텍스처 맵.
        ProgramUPtr mProgram;

        /// @brief 단순 셰이더 프로그램 (`simple.vs/fs`). 광원 큐브 등 *라이팅 무관* 객체 그릴 때 사용 — color uniform 직접 출력.
        ProgramUPtr mSimpleProgram;

        ProgramUPtr mTextureProgram;

        /// @brief 박스(큐브) 메시 — 기본 씬 오브젝트. @c Mesh::CreateBox() 로 Init 에서 생성.
        MeshUPtr mBox;

        MeshUPtr mPlane;

        /// @brief 큐브 회전 애니메이션 활성. ImGui Checkbox 토글 — false 시 모든 큐브가 정지.
        bool mAnimation{true};

        bool mFlashLightMode{true};

        /// @brief glDepthFunc 비교 연산자 선택 인덱스 — ImGui Combo 가 갱신.
        ///        Render() 의 DEPTH_FUNC[] 룩업 테이블 인덱스. 기본 2 = GL_LESS (OpenGL 기본값).
        int mDepthFuncIndex{2};

        /// @brief Texture/Material/Model 을 이름 키로 캐시·조회하는 리소스 레지스트리.
        ResourceRegistryUPtr mRM;

        /// @brief 현재 프레임버퍼 너비. @ref Reshape 가 갱신, @ref Render 가 projection 행렬 산출에 사용.
        int mWidth{640};

        /// @brief 현재 프레임버퍼 높이.
        int mHeight{480};

        /// @brief 직전 프레임 마우스 위치 — 회전 델타 계산용. @ref MouseButton(LEFT, PRESS) 에서 초기화.
        glm::vec2 mPrevMousePos{glm::vec2(0.0f)};

        /// @brief 카메라 상태(POD-like). View/Projection 행렬 산출 + 입력 메서드들이 직접 갱신.
        Camera mCamera;

        // === ImGui / 프레임 클리어 ===
        /// @brief @c glClearColor 인자. ImGui ColorEdit3 위젯이 직접 갱신.
        glm::vec4 mClearColor{glm::vec4(0.1f, 0.2f, 0.3f, 0.0f)};

        // === 라이팅 (Multi Light Caster — Directional + Point[N] + Spot) ===

        /// @brief 평행광 1개 — 전역 ambient + 태양광 역할. 셰이더 uniform `dirLight.*`.
        DirLight mDirLight;

        /// @brief 점광원 2개 — 위치 + 거리감쇠. 셰이더 uniform `pointLights[i].*`.
        ///        배열 크기는 셰이더의 @c NUM_POINT_LIGHTS 와 동기 (현재 2).
        PointLight mPointLights[2];

        /// @brief 스포트라이트 1개 — 점광원 + 콘 cutoff. 셰이더 uniform `spotLight.*`.
        SpotLight mSpotLight;

        // === Light 활성 토글 — 셰이더 uniform `*Enabled` 로 전송, ImGui 체크박스가 갱신. ===
        /// @brief DirLight 사용 여부 — false 면 셰이더가 dirLight 항을 누적하지 않음.
        bool mDirLightEnabled{true};
        /// @brief PointLight 슬롯별 사용 여부 — 슬롯 단위 독립 토글.
        bool mPointLightsEnabled[2]{true, true};
        /// @brief SpotLight 사용 여부.
        bool mSpotLightEnabled{true};

        const char *STR_IMAGE_DARK_GRAY = "image_dark_gray";
        const char *STR_IMAGE_GRAY = "image_gray";
        const char *STR_IMAGE_MARBLE = "image_marble";
        const char *STR_IMAGE_BOX1_DIFFUSE = "image_box1_diffuse";
        const char *STR_IMAGE_BOX2_DIFFUSE = "image_box2_diffuse";
        const char *STR_IMAGE_BOX2_SPECULAR = "image_box2_specular";
        const char *STR_IMAGE_PLANE = "image_plane";

        const char *STR_TEXTURE_DARK_GRAY = "texture_dark_gray";
        const char *STR_TEXTURE_GRAY = "texture_gray";
        const char *STR_TEXTURE_MARBLE = "texture_marble";
        const char *STR_TEXTURE_BOX1_DIFFUSE = "texture_box1_diffuse";
        const char *STR_TEXTURE_BOX2_DIFFUSE = "texture_box2_diffuse";
        const char *STR_TEXTURE_BOX2_SPECULAR = "texture_box2_specular";
        const char *STR_TEXTURE_WINDOW = "texture_window";

        const char *STR_MATERIAL_PLANE = "material_plane";
        const char *STR_MATERIAL_BOX1 = "material_box1";
        const char *STR_MATERIAL_BOX2 = "material_box2";

        // depth test 비교 연산자 선택 — 라벨 배열 순서는 아래 DEPTH_FUNC[] 와 동일해야 함.
        // Depth Test를 꺼야하는 상황은? -> ImGUI를 사용할때 이다.
        // Depth 독립적으로 항상 앞으로 그려야 한다. 혹은 항상 뒤로 그려야 한다 할때.
        // glClearDepth(1.0f)
        //      제일 가까운애가 0, 제일 멀리있는게 1
        //      GL_LESS : 1보다 더 작은애를 먼저 그리게 한다.
        // ┌───────┬─────────────┬──────────────────────────────────┐
        // │ 인덱스 │ 값          │ 의미                             │
        // ├───────┼─────────────┼──────────────────────────────────┤
        // │ 0     │ GL_ALWAYS   │ 항상 통과 (depth test 무력화 효과) │
        // │ 1     │ GL_NEVER    │ 항상 실패 (아무것도 안 그려짐)    │
        // │ 2     │ GL_LESS     │ 더 가까우면 통과 (기본값)        │
        // │ 3     │ GL_LEQUAL   │ 같거나 가까우면 통과             │
        // │ 4     │ GL_GREATER  │ 더 멀면 통과                     │
        // │ 5     │ GL_GEQUAL   │ 같거나 멀면 통과                 │
        // │ 6     │ GL_EQUAL    │ 깊이 같을 때만                   │
        // │ 7     │ GL_NOTEQUAL │ 깊이 다를 때만                   │
        // └───────┴─────────────┴──────────────────────────────────┘
        const char *const DEPTH_FUNC_LABELS[8] = {
            "GL_ALWAYS", "GL_NEVER",
            "GL_LESS", "GL_LEQUAL",
            "GL_GREATER", "GL_GEQUAL",
            "GL_EQUAL", "GL_NOTEQUAL"};

        const uint32_t DEPTH_FUNC[8] = {
            GL_ALWAYS, GL_NEVER,
            GL_LESS, GL_LEQUAL,
            GL_GREATER, GL_GEQUAL,
            GL_EQUAL, GL_NOTEQUAL};
    };
}

#endif // __SJH_CONTEXT_H__
