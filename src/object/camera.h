#ifndef __SJH_CAMERA_H__
#define __SJH_CAMERA_H__

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace SJH
{
    /**
     * @brief 3D 씬에서 시점(view) + 투영(projection) 행렬을 산출하는 카메라 상태 컨테이너.
     *
     * @details
     *  ### 책임
     *  - 카메라 위치/방향 상태 보관 (@c mPos / @c mTarget / @c mCamUp)
     *  - Yaw/Pitch 회전 각도 보관 (@c mEulerYaw / @c mEulerPitch — 단위: degree)
     *  - 원근 투영 파라미터 보관 (@c mFov / @c mAspect / @c mNearPlane / @c mFarPlane)
     *  - 입력 활성 플래그 (@c mIsCamControl)
     *  - View / Projection 행렬 합성 (@ref GetForwardViewMatrix / @ref GetLookAtViewMatrix / @ref GetProjMatrix)
     *
     *  ### 비-책임
     *  - ❌ 입력 처리 — Context 가 본 객체의 필드를 직접 갱신.
     *  - ❌ 자원 보유 — POD-like 상태 컨테이너. 멤버 모두 public 으로 직접 접근 허용 *(의도된 디자인 — 추후 변경 예정)*.
     *
     *  ### 좌표계 가정
     *  - Right-handed, +Y up, -Z forward (OpenGL 기본).
     *  - 초기 위치 @c (0,0,3), 초기 front @c (0,0,-1) → 원점을 바라봄.
     *
     *  ### 명명 컨벤션
     *  - 멤버는 @c m-prefix camelCase — 코드베이스 다른 클래스 (Program, Texture 등) 와 통일.
     *  - 메서드는 PascalCase.
     */
    class Camera
    {
    public:
        // === 위치/방향 상태 ===
        /// @brief 월드 공간 카메라 위치. 기본값 @c (0,0,3).
        glm::vec3 Pos    = glm::vec3(0.0f, 0.0f, 3.0f);
        /// @brief look-at 모드의 응시 지점. forward 모드에서는 무시.
        glm::vec3 Target = glm::vec3(0.0f, 0.0f, 0.0f);
        /// @brief 카메라의 위 방향 단위 벡터. 기본값 @c (0,1,0).
        glm::vec3 CamUp  = glm::vec3(0.0f, 1.0f, 0.0f);

        // === 입력 / 회전 ===
        /// @brief 입력으로 카메라를 조작 중인지 여부. 마우스 좌클릭 PRESS/RELEASE 가 토글.
        bool  IsCamControl{false};
        /// @brief Pitch (X축 회전, 위/아래). 단위: degree. 호출자가 @c [-89, 89] 로 클램프.
        float EulerPitch{0.0f};
        /// @brief Yaw (Y축 회전, 좌/우). 단위: degree. 호출자가 @c [0, 360) 으로 정규화.
        float EulerYaw{0.0f};

        // === 원근 투영 파라미터 ===
        /// @brief 수직 시야각(Field of View). 단위: degree.
        float Fov       = 60.0f;
        /// @brief 종횡비(width/height). @ref SetAspect 로 갱신, @ref GetProjMatrix 가 사용.
        float Aspect    = 1.0f;
        /// @brief 가까운 클리핑 평면. @c 0 보다 커야 함.
        float NearPlane = 0.1f;
        /// @brief 먼 클리핑 평면. @c mNearPlane 보다 커야 함.
        float FarPlane  = 1000.0f;

        // === 게터 — 모두 @c const, 멤버 변경 없음 ===

        /**
         * @brief 현재 Yaw/Pitch 로부터 카메라 *전방* 단위 벡터 산출.
         * @details 합성 순서 @c yawMat*pitchMat — Pitch 를 *먼저* (로컬 right 축 기준) 적용한 뒤
         *          Yaw 를 (월드 up 축 기준) 적용하는 FPS 카메라 표준 순서.
         *          결과 front 의 y성분 = @c sin(pitch) 로 yaw 와 독립.
         * @warning 순서를 뒤집으면 (@c pitchMat*yawMat) Pitch 가 *월드 X축* 기준으로 돌아
         *          y성분이 @c cos(yaw)*sin(pitch) 가 되어 — yaw 에 따라 상하 회전량이
         *          달라지는 yaw-pitch 결합 버그가 생긴다.
         */
        glm::vec3 GetFront() const
        {
            const auto pitchMat = glm::rotate(glm::mat4(1.0f),
                                              glm::radians(EulerPitch),
                                              glm::vec3(1.0f, 0.0f, 0.0f));
            const auto yawMat   = glm::rotate(glm::mat4(1.0f),
                                              glm::radians(EulerYaw),
                                              glm::vec3(0.0f, 1.0f, 0.0f));
            return glm::vec3(yawMat * pitchMat * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
        }

        /**
         * @brief look-at 모드 view 행렬 — @c mTarget 응시.
         * @details forward 모드와 동시 사용 금지 (모드 추적 안 함 — 호출자 책임).
         *          *추후 mode enum 도입 예정*.
         */
        glm::mat4 GetLookAtViewMatrix() const
        {
            return glm::lookAt(Pos, Target, CamUp);
        }

        /**
         * @brief forward 모드 view 행렬 — @c mEulerYaw / @c mEulerPitch 기반 front 벡터 응시.
         * @note  매 프레임 @c Context::Render 에서 호출.
         */
        glm::mat4 GetForwardViewMatrix() const
        {
            return glm::lookAt(Pos, Pos + GetFront(), CamUp);
        }

        /**
         * @brief Perspective Projection 행렬 산출 — 멤버 @c mAspect 사용.
         * @return 카메라 공간을 클립 공간으로 투영하는 4x4 행렬.
         * @pre @c mAspect 가 유효 — @ref SetAspect 가 호출되어 있어야 함.
         */
        glm::mat4 GetProjMatrix() const
        {
            return glm::perspective(glm::radians(Fov), Aspect, NearPlane, FarPlane);
        }

        // === 세터 — 멤버 갱신 (의도가 명시적 — Get* 와 분리) ===

        /**
         * @brief 뷰포트 크기로부터 종횡비 갱신 — 윈도우 리사이즈 시 호출.
         * @param width   뷰포트 너비 (픽셀, > 0 가정).
         * @param height  뷰포트 높이 (픽셀). @c 0 일 때 (윈도우 최소화 등) 는 무시 — 이전 값 유지.
         * @details height==0 이면 division by zero 위험 → 본 함수가 가드. 호출자는 매번 안전하게 호출 가능.
         */
        void SetAspect(float width, float height)
        {
            if (height <= 0.0f)
                return;   // 최소화 등 — Aspect 변경 안 함, 이전 유효 값 유지
            Aspect = width / height;
        }
    };
} // namespace SJH

#endif //__SJH_CAMERA_H__
