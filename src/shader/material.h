/**
 * @file material.h
 * @brief 표면 머티리얼 정의 — 디퓨즈/스페큘러 텍스처 *이름 키* + Phong shininess 지수.
 *
 * @details
 *  ### 책임
 *  - 텍스처 *논리 이름* 보관 (실제 GPU 텍스처는 @c SJH::ResourceManagement 가 캐시).
 *  - Phong shininess 지수 (specular highlight 집중도).
 *
 *  ### 변경 이력
 *  - **Phase 12 (02bd90e)**: 처음 도입 — `mAmbient` / `mDiffuse` / `mSpecular` *색상 vec3* 보관.
 *  - **Phase 14 (2444fbe)**: 텍스처 기반으로 완전히 교체 — 색상 vec3 제거, 텍스처 이름 키 추가.
 *    Phong 의 ambient/diffuse 항은 디퓨즈 텍스처 색상으로, specular 항은 스페큘러 맵으로 대체.
 *
 *  ### 비-책임
 *  - ❌ 텍스처 *데이터 보유* — @c SJH::ResourceManagement (이름 → @c Texture 캐시) 의 책임.
 *  - ❌ uniform 전송 — @c Context::Render 가 @c Uniforms::SetInt 로 sampler 슬롯, @c SetFloat 로 shininess 전달.
 *
 * @note 현재 @c SJH:: 네임스페이스 *외부* 에 정의 — 코드베이스 다른 클래스와 일관성 어긋남.
 *       향후 @c SJH:: 로 이동 + ResourceManagement 의존 명시화 예정.
 */
#ifndef __SJH_MATERIAL_H__
#define __SJH_MATERIAL_H__

#include <glm/glm.hpp>
#include <string>

/// @brief 텍스처 기반 Phong 머티리얼 (디퓨즈 + 스페큘러 맵 + shininess).
class Material
{
public:
    /// @brief 디퓨즈 맵 텍스처 이름 키.
    /// @details @c ResourceManagement::LoadTextureWithName 으로 조회. ambient + diffuse 항 모두 본 맵 색상 사용.
    std::string mDiffuseTextureName;

    /// @brief 스페큘러 맵 텍스처 이름 키. 픽셀별 스페큘러 강도 (예: 금속 vs 비금속 영역 구분).
    /// @details 이름이 비어 있거나 매칭 실패 시 셰이더 측에서 default(검정 또는 회색) 사용을 가정.
    std::string mSpecularTextureName;

    /// @brief Phong shininess 지수 — 큰 값일수록 좁고 날카로운 하이라이트.
    /// @details 셰이더 uniform `material.shininess`. 일반 범위 @c [2, 256], 기본 @c 32.
    float mShininess{32.0f};
};

#endif // __SJH_MATERIAL_H__
