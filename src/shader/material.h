/**
 * @file material.h
 * @brief 표면 머티리얼 — 디퓨즈/스페큘러 텍스처 *이름 키* + 해석된 GL 핸들/유닛 + Phong shininess.
 *
 * @details
 *  ### 책임
 *  - 텍스처 *논리 이름* 보관 (실제 GPU 텍스처는 @c SJH::ResourceManagement 가 캐시).
 *  - 이름으로부터 *해석된* GL 핸들 + sampler 유닛 보관 (Context 가 resolve 후 설정).
 *  - Phong shininess 지수 (specular highlight 집중도).
 *
 *  ### 캡슐화 — 왜 getter/setter (직접 대입 차단)
 *  - 텍스처 *이름* 과 *해석된 핸들* 은 *짝* — 이름을 함부로 바꾸면 핸들이 stale.
 *    → 이름 setter (@ref SetDiffuseTextureName / @ref SetSpecularTextureName) 가 *해석 캐시 무효화*.
 *  - shininess 는 유효 범위 권장 @c [2, 256] — @ref SetShininess 가 clamp.
 *  - 모든 멤버 private — 외부는 getter/setter 경유. @c &mMaterial.mShininess 같은 raw 대입 불가.
 *
 *  ### 변경 이력
 *  - **Phase 12 (02bd90e)**: 처음 도입 — `mAmbient` / `mDiffuse` / `mSpecular` *색상 vec3* 보관.
 *  - **Phase 14 (2444fbe)**: 텍스처 기반으로 완전히 교체 — 색상 vec3 제거, 텍스처 이름 키 추가.
 *  - **(이번 리팩토링)**: public 멤버 → private + getter/setter. 이름 변경 시 핸들 무효화.
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

#include <glad/glad.h>
#include <string>

/// @brief 텍스처 기반 Phong 머티리얼 (디퓨즈 + 스페큘러 맵 + shininess).
class Material
{
public:
    // === 초기화 진입점 — 텍스처 이름 키 설정을 감싼다 ===

    /// @brief 디퓨즈 + 스페큘러 텍스처 이름 키를 한 번에 설정.
    void SetTextureNames(const std::string &diffuseName, const std::string &specularName)
    {
        SetDiffuseTextureName(diffuseName);
        SetSpecularTextureName(specularName);
    }

    /// @brief 디퓨즈 맵 이름 키 갱신 — *해석된 핸들 무효화* (재 resolve 필요).
    /// @details 이름이 바뀌면 기존 @c mDiffuseTexture 핸들은 stale → 0 으로 되돌려 강제 재해석 유도.
    void SetDiffuseTextureName(const std::string &name)
    {
        mDiffuseTextureName = name;
        mDiffuseTexture     = 0;
    }

    /// @brief 스페큘러 맵 이름 키 갱신 — *해석된 핸들 무효화*.
    void SetSpecularTextureName(const std::string &name)
    {
        mSpecularTextureName = name;
        mSpecularTexture     = 0;
    }

    /// @brief 이름으로부터 *해석된* GL 핸들 + sampler 유닛 설정.
    /// @details @c ResourceManagement::LoadTextureWithName 으로 이름 → 핸들 해석 후 Context 가 호출.
    void SetResolvedTextures(GLuint diffuseHandle, GLint diffuseUnit,
                             GLuint specularHandle, GLint specularUnit)
    {
        mDiffuseTexture  = diffuseHandle;
        mDiffuseUnit     = diffuseUnit;
        mSpecularTexture = specularHandle;
        mSpecularUnit    = specularUnit;
    }

    /// @brief Phong shininess 지수 설정 — @c [2, 256] 으로 clamp (기본 @c 32).
    void SetShininess(float v)
    {
        mShininess = (v < 2.0f) ? 2.0f : (v > 256.0f ? 256.0f : v);
    }

    // === Getters — 모두 const, 읽기 전용 ===
    const std::string &GetDiffuseTextureName()  const { return mDiffuseTextureName; }
    const std::string &GetSpecularTextureName() const { return mSpecularTextureName; }
    GLuint GetDiffuseTexture()  const { return mDiffuseTexture; }
    GLuint GetSpecularTexture() const { return mSpecularTexture; }
    GLint  GetDiffuseUnit()  const { return mDiffuseUnit; }
    GLint  GetSpecularUnit() const { return mSpecularUnit; }
    float  GetShininess()    const { return mShininess; }

    /// @brief 디퓨즈 핸들이 해석된 상태인지 (0 이면 미해석 — @c SetResolvedTextures 호출 필요).
    bool IsResolved() const { return mDiffuseTexture != 0; }

private:
    std::string mDiffuseTextureName;   ///< 디퓨즈 맵 이름 키 — @c ResourceManagement::LoadTextureWithName 조회
    std::string mSpecularTextureName;  ///< 스페큘러 맵 이름 키 — 비어 있으면 셰이더 측 default 가정
    GLuint mDiffuseTexture{0};         ///< 이름으로부터 해석된 GL 텍스처 핸들 (0 = 미해석)
    GLuint mSpecularTexture{0};        ///< 스페큘러 맵 GL 핸들
    GLint  mDiffuseUnit{0};            ///< 디퓨즈 sampler2D 에 넣을 텍스처 이미지 유닛 번호
    GLint  mSpecularUnit{1};           ///< 스페큘러 sampler2D 의 유닛 번호
    float  mShininess{32.0f};          ///< Phong shininess — 셰이더 uniform `material.shininess`. 권장 [2,256]
};

#endif // __SJH_MATERIAL_H__
