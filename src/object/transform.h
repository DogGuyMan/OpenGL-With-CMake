/**
 * @file transform.h
 * @brief 씬 트랜스폼 — 이름 태그 인터페이스 + TRS 계층 구조 + UV 변환.
 *
 * @details
 *  ### 책임
 *  - @ref SJH::INameTagInterface — 이름으로 식별 가능한 씬 오브젝트의 순수 인터페이스.
 *  - @ref SJH::Transform — TRS(Translate/Rotate/Scale) 값 + 부모-자식 계층 구조 + 모델 행렬 산출.
 *  - @ref SJH::UVTransform — UV 오프셋/스케일/회전을 묶은 경량 구조체.
 *
 *  ### 비-책임
 *  - ❌ 렌더링 / 드로우콜 — Context::Render 가 담당.
 *  - ❌ GL uniform 전송 — GetModelMatrix() 결과를 Context 가 Uniforms::SetMat4 로 push.
 */

#ifndef __SJH_TRANSFORM_H__
#define __SJH_TRANSFORM_H__

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace SJH
{
    /**
     * @brief 이름으로 식별 가능한 씬 오브젝트의 순수 인터페이스.
     * @details 복사/이동 금지 (이름 단일성 보장). 상속자는 @ref GetName 을 구현해야 함.
     */
    class INameTagInterface
    {
    protected:
        INameTagInterface() = default;

    public:
        virtual ~INameTagInterface() = default;
        INameTagInterface(const INameTagInterface &) = delete;             ///< 이름 단일성 — 복사 금지.
        INameTagInterface operator=(const INameTagInterface &) = delete;
        INameTagInterface(INameTagInterface &&) = delete;
        INameTagInterface operator=(INameTagInterface &) = delete;
        /// @brief 오브젝트의 논리 이름 반환 (읽기 전용).
        virtual const std::string &GetName() const = 0;
    };

    /**
     * @brief TRS(Translate/Rotate/Scale) 기반 트랜스폼 — 부모-자식 계층 구조 지원.
     * @details
     *  - 부모(@ref Parent)가 있으면 @ref GetModelMatrix 가 부모 행렬을 재귀 합성.
     *  - 회전은 오일러 각(X→Y→Z 순서) — 짐벌락 주의.
     *  - 자식 등록은 @ref Children 맵에 이름 키로 보관 (비소유 관찰자).
     * @note 현재 @c Context 에서는 직접 사용되지 않음 — 향후 씬 그래프 확장을 위한 기반.
     */
    class Transform : public INameTagInterface
    {
    public:
        std::string Name;  ///< 오브젝트 논리 이름 — @ref INameTagInterface::GetName 반환값.

        glm::vec3 Translate = glm::vec3(0.0f, 0.0f, 0.0f); ///< 이동량 (world space offset).
        glm::vec3 EulerRot = glm::vec3(0.0f, 0.0f, 0.0f);  ///< 오일러 회전각 (degrees, XYZ 순서).
        glm::vec3 Scale = glm::vec3(1.0f, 1.0f, 1.0f);      ///< 스케일 팩터.

        Transform *Parent = nullptr;                              ///< 부모 트랜스폼 (비소유 관찰자). nullptr 이면 루트.
        std::unordered_map<std::string, Transform *> Children;   ///< 이름 → 자식 트랜스폼 (비소유 관찰자).

        virtual const std::string &GetName() const override
        {
            return Name;
        }

        /**
         * @brief 로컬 TRS 로 모델 행렬 산출 — 부모가 있으면 재귀 합성.
         * @return 최종 월드 공간 모델 행렬 (parent × local).
         */
        glm::mat4 GetModelMatrix() const
        {
            glm::mat4 local =
                glm::translate(glm::mat4(1.0), Translate) *
                glm::rotate(glm::mat4(1.0f), glm::radians(EulerRot[2]), glm::vec3(0.0f, 0.0f, 1.0f)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(EulerRot[1]), glm::vec3(0.0f, 1.0f, 0.0f)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(EulerRot[0]), glm::vec3(1.0f, 0.0f, 0.0f)) *
                glm::scale(glm::mat4(1.0f), Scale);
            if (Parent == nullptr)
                return local;
            return Parent->GetModelMatrix() * local;
        }
    };

    /// @brief UV 좌표계 변환 (오프셋 + 스케일 + 회전) 경량 구조체.
    class UVTransform
    {
    public:
        glm::vec2 Offset{0.0f, 0.0f}; ///< UV 오프셋 — 텍스처 스크롤 효과.
        glm::vec2 Scale{1.0f, 1.0f};  ///< UV 스케일 — 타일링 배수.
        float RotationDeg = 0.0f;     ///< UV 회전각 (degree).
    };
}
#endif //__SJH_TRANSFORM_H__
