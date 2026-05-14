/**
 * @file light.h
 * @brief 점 광원 — 위치 + Phong 3항 (ambient / diffuse / specular) 색상.
 *
 * @details
 *  ### 책임
 *  - 광원 *위치* 와 Phong 라이팅 모델의 *3개 항 색상* 보관.
 *  - 셰이더 uniform `light.position` / `light.ambient` / `light.diffuse` / `light.specular` 로 전송될 데이터 컨테이너.
 *
 *  ### Phong 3항의 직관
 *  - **ambient**: 광원과 무관한 *기본 밝기* — 그림자 영역도 완전히 검지 않게.
 *  - **diffuse**: 표면 normal 과 광원 방향의 cos 으로 감쇠 — *주된* 밝기 항.
 *  - **specular**: 시선 방향과 반사 벡터의 cos^shininess — *하이라이트*.
 *
 *  ### 변경 이력
 *  - **Phase 10 (306d9f4)**: Context 가 직접 라이팅 멤버 보유 (`mLightPos` / `mLightColor` / `mAmbientStrength` 등 8개).
 *  - **Phase 12 (02bd90e)**: 본 클래스로 분리 — Phong 3항을 색상 vec3 로 표현.
 *
 *  ### 비-책임
 *  - ❌ 셰이더 uniform 전송 — @c Context::Render 가 @c Uniforms::SetVec3 로 직접 push.
 *  - ❌ 광원 *타입* (점/방향/스포트) 분기 — 현재 점광원만 지원. 추후 enum + subclass 도입 예정.
 *
 * @note 현재 @c SJH:: 네임스페이스 *외부* 에 정의 — 코드베이스 다른 클래스와 일관성 어긋남.
 *       향후 @c SJH:: 로 이동 + 광원 타입 enum 도입 예정.
 */

#ifndef __SJH_LIGHT_H__
#define __SJH_LIGHT_H__
#include <glm/glm.hpp>

namespace SJH
{
    /// @brief 점 광원 + Phong 3항 색상 컨테이너.
    class Light
    {
    public:
        /// @brief 점 광원 월드 좌표. 셰이더 uniform `light.position`. ImGui DragFloat3 위젯이 갱신.
        glm::vec3 mPos{glm::vec3(3.0f, 3.0f, 3.0f)};

        /// @brief Ambient 항 색상 (RGB, 0~1). 셰이더 uniform `light.ambient`.
        /// @details 광원과 무관한 기본 밝기. 일반적으로 매우 작은 값 (예: @c (0.1, 0.1, 0.1)) 으로 그림자 영역에도 약간의 색.
        glm::vec3 mAmbient{glm::vec3(0.1f, 0.1f, 0.1f)};

        /// @brief Diffuse 항 색상 (RGB, 0~1). 셰이더 uniform `light.diffuse`.
        /// @details Lambertian 항의 광원 색. 광원의 *주된 색상* — 일반적으로 흰색 근처.
        glm::vec3 mDiffuse{glm::vec3(0.5f, 0.5f, 0.5f)};

        /// @brief Specular 항 색상 (RGB, 0~1). 셰이더 uniform `light.specular`.
        /// @details 하이라이트 색. 일반적으로 흰색 — 금속이 아닌 표면은 광원 색을 그대로 반사.
        glm::vec3 mSpecular{glm::vec3(1.0f, 1.0f, 1.0f)};
    };

    /// @brief 평행광 (태양 등) — 위치 없이 *방향* 만 가짐. 거리 감쇠 없음.
    /// @note 셰이더 구조체 `DirLight` 와 1:1 매핑 — 같은 이름 유지.
    class DirLight
    {
    public:
        glm::vec3 mDirection{glm::vec3(-0.2f, -1.0f, -0.3f)};

        /// @brief Ambient 항 색상 (RGB, 0~1). 셰이더 uniform `light.ambient`.
        /// @details 광원과 무관한 기본 밝기. 일반적으로 매우 작은 값 (예: @c (0.1, 0.1, 0.1)) 으로 그림자 영역에도 약간의 색.
        glm::vec3 mAmbient{glm::vec3(0.1f, 0.1f, 0.1f)};

        /// @brief Diffuse 항 색상 (RGB, 0~1). 셰이더 uniform `light.diffuse`.
        /// @details Lambertian 항의 광원 색. 광원의 *주된 색상* — 일반적으로 흰색 근처.
        glm::vec3 mDiffuse{glm::vec3(0.5f, 0.5f, 0.5f)};

        /// @brief Specular 항 색상 (RGB, 0~1). 셰이더 uniform `light.specular`.
        /// @details 하이라이트 색. 일반적으로 흰색 — 금속이 아닌 표면은 광원 색을 그대로 반사.
        glm::vec3 mSpecular{glm::vec3(1.0f, 1.0f, 1.0f)};
    };

    class PointLight
    {
    public:
        /// @brief 거리 감쇠 산출 기준 도달 거리. 셰이더 uniform `pointLights[i].attenuation`(vec3) 는 @ref GetAttenuationCoeff(mDistance) 로 도출.
        float mDistance{32.0f};
        /// @brief 점 광원 월드 좌표. 셰이더 uniform `pointLights[i].position`. ImGui DragFloat3 위젯이 갱신.
        glm::vec3 mPos{glm::vec3(3.0f, 3.0f, 3.0f)};

        /// @brief Ambient 항 색상 (RGB, 0~1). 셰이더 uniform `light.ambient`.
        /// @details 광원과 무관한 기본 밝기. 일반적으로 매우 작은 값 (예: @c (0.1, 0.1, 0.1)) 으로 그림자 영역에도 약간의 색.
        glm::vec3 mAmbient{glm::vec3(0.1f, 0.1f, 0.1f)};

        /// @brief Diffuse 항 색상 (RGB, 0~1). 셰이더 uniform `light.diffuse`.
        /// @details Lambertian 항의 광원 색. 광원의 *주된 색상* — 일반적으로 흰색 근처.
        glm::vec3 mDiffuse{glm::vec3(0.5f, 0.5f, 0.5f)};

        /// @brief Specular 항 색상 (RGB, 0~1). 셰이더 uniform `light.specular`.
        /// @details 하이라이트 색. 일반적으로 흰색 — 금속이 아닌 표면은 광원 색을 그대로 반사.
        glm::vec3 mSpecular{glm::vec3(1.0f, 1.0f, 1.0f)};
    };

    /// @brief 스포트라이트 — 위치 + 콘 축 방향 + inner/outer 컷오프 + 거리 감쇠 + Phong 3항.
    /// @details PointLight 에 방향(@ref mDirection)과 콘 컷오프(@ref mCutoffAngleDeg, @ref mOuterCutoffAngleDeg)
    ///          가 추가된 형태. 콘 안쪽은 fully lit, 바깥쪽은 fully dark,
    ///          inner~outer 구간은 부드럽게 감쇠(soft edge) 시키는 데 사용.
    class SpotLight
    {
    public:
        /// @brief 광원 월드 좌표. 셰이더 uniform `light.position`. ImGui DragFloat3 위젯이 갱신.
        glm::vec3 mPos{glm::vec3(3.0f, 3.0f, 3.0f)};

        /// @brief 스포트 콘 축 방향 (world space, 정규화 권장). 셰이더 uniform `light.direction`.
        /// @details `normalize(-mDirection)` 과 lightDir 의 dot 으로 `theta` 계산 → cutoff 비교.
        glm::vec3 mDirection{glm::vec3(0.0f, -1.0f, 0.0f)};

        /// @brief 콘 안쪽 컷오프 각도 (degree). 이 각도 이내는 fully lit.
        /// @details degree 로 보관 — 셰이더 송신 시 `cosf(glm::radians(mCutoffAngleDeg))` 변환 후 push.
        float mCutoffAngleDeg{12.5f};

        /// @brief 콘 바깥쪽 컷오프 각도 (degree). 이 각도 바깥은 fully dark.
        /// @details mCutoffAngleDeg ~ mOuterCutoffAngleDeg 구간은 smoothstep 으로 부드럽게 감쇠.
        ///          현재 셰이더(lighting.fs) 는 `light.cutoff` 단일 float 만 사용 — outer 는 향후 확장용.
        float mOuterCutoffAngleDeg{17.5f};

        /// @brief 거리 감쇠 산출 기준 도달 거리.
        /// @details 셰이더 uniform `light.attenuation`(vec3 = Kc, Kl, Kq) 는 @ref GetAttenuationCoeff(mDistance) 로 도출.
        float mDistance{32.0f};

        /// @brief Ambient 항 색상 (RGB, 0~1). 셰이더 uniform `light.ambient`.
        glm::vec3 mAmbient{glm::vec3(0.1f, 0.1f, 0.1f)};

        /// @brief Diffuse 항 색상 (RGB, 0~1). 셰이더 uniform `light.diffuse`.
        glm::vec3 mDiffuse{glm::vec3(0.5f, 0.5f, 0.5f)};

        /// @brief Specular 항 색상 (RGB, 0~1). 셰이더 uniform `light.specular`.
        glm::vec3 mSpecular{glm::vec3(1.0f, 1.0f, 1.0f)};
    };

    static glm::vec3 GetAttenuationCoeff(float distance)
    {
        const auto linear_coeff = glm::vec4(
            8.4523112e-05, 4.4712582e+00, -1.8516388e+00, 3.3955811e+01);
        const auto quad_coeff = glm::vec4(
            -7.6103583e-04, 9.0120201e+00, -1.1618500e+01, 1.0000464e+02);

        float kc = 1.0f;
        float d = 1.0f / distance;
        auto dvec = glm::vec4(1.0f, d, d * d, d * d * d);
        float kl = glm::dot(linear_coeff, dvec);
        float kq = glm::dot(quad_coeff, dvec);

        return glm::vec3(kc, glm::max(kl, 0.0f), glm::max(kq * kq, 0.0f));
    }

}
#endif // __SJH_LIGHT_H__
