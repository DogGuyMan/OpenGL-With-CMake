/**
 * @file material.h
 * @brief 표면 머티리얼 — 디퓨즈/스페큘러 텍스처 *이름 키* + 해석된 GL 핸들/유닛 + Phong shininess.
 *
 * @details
 *  ### 책임
 *  - 텍스처 *논리 이름* 보관 (실제 GPU 텍스처는 @c SJH::ResourceRegistry 가 캐시).
 *  - 이름으로부터 *해석된* GL 핸들 + sampler 유닛 보관 (Context 가 resolve 후 설정).
 *  - Phong shininess 지수 (specular highlight 집중도).
 *
 *  ### 캡슐화 — 왜 getter/setter (직접 대입 차단)
 *  - 텍스처 *이름* 과 *해석된 핸들* 은 *짝* — 이름을 함부로 바꾸면 핸들이 stale.
 *    → 이름 setter (@c SetDiffuseTextureName / @c SetSpecularTextureName) 가 *해석 캐시 무효화*.
 *  - shininess 는 유효 범위 권장 @c [2, 256] — @c SetShininess 가 clamp.
 *  - 모든 멤버 private — 외부는 getter/setter 경유. @c &mMaterial.mShininess 같은 raw 대입 불가.
 *
 *  ### 변경 이력
 *  - **Phase 12 (02bd90e)**: 처음 도입 — `mAmbient` / `mDiffuse` / `mSpecular` *색상 vec3* 보관.
 *  - **Phase 14 (2444fbe)**: 텍스처 기반으로 완전히 교체 — 색상 vec3 제거, 텍스처 이름 키 추가.
 *  - **(이번 리팩토링)**: public 멤버 → private + getter/setter. 이름 변경 시 핸들 무효화.
 *
 *  ### 비-책임
 *  - ❌ 텍스처 *데이터 보유* — @c SJH::ResourceRegistry (이름 → @c Texture 캐시) 의 책임.
 *  - ❌ uniform 전송 — @c Context::Render 가 @c Uniforms::SetInt 로 sampler 슬롯, @c SetFloat 로 shininess 전달.
 *
 */
#ifndef __SJH_MATERIAL_H__
#define __SJH_MATERIAL_H__

#include "program/program_uniforms.h"
#include "resource_registry/texture.h" // SetToProgram 이 Texture::Bind() 호출 → 완전한 타입 필요
#include "common/common.h" // CLASS_PTR 매크로 + 스마트 포인터 별칭 alias
#include <glad/glad.h>
#include <string>

namespace SJH
{
    // Texture 는 resource_registry/texture.h 에서 완전 정의 (SetToProgram 이 Bind() 호출).
    CLASS_PTR(Material);
    /// @brief 텍스처 기반 Phong 머티리얼 (디퓨즈 + 스페큘러 맵 + shininess).
    class Material
    {
    public:
        static MaterialUPtr Create()
        {
            return MaterialUPtr(new Material());
        }
        // === 초기화 진입점 — 텍스처 이름 키 설정을 감싼다 ===

        /// @brief 디퓨즈 + 스페큘러 텍스처 이름 키를 한 번에 설정.
        void SetTextureNames(const std::string &diffuseName, const std::string &specularName)
        {
            SetDiffuseTextureName(diffuseName);
            SetSpecularTextureName(specularName);
        }

        /// @brief 디퓨즈 맵 이름 키 갱신 — *해석된 핸들 무효화* (재 resolve 필요).
        /// @details 이름이 바뀌면 기존 @c mDiffuseTexture 포인터는 stale → nullptr 로 되돌려 강제 재해석 유도.
        void SetDiffuseTextureName(const std::string &name)
        {
            mDiffuseTextureName = name;
            mDiffuseTexture = nullptr;
        }

        /// @brief 스페큘러 맵 이름 키 갱신 — *해석된 핸들 무효화*.
        void SetSpecularTextureName(const std::string &name)
        {
            mSpecularTextureName = name;
            mSpecularTexture = nullptr;
        }

        /// @brief 이름으로부터 *해석된* 텍스처 관찰자 + sampler 유닛 설정.
        /// @details 텍스처 소유자(ResourceRegistry 또는 Model)가 Material 보다 오래 산다는 불변식 전제.
        void SetResolvedTextures(const Texture *diffuse, GLint diffuseUnit,
                                 const Texture *specular, GLint specularUnit)
        {
            mDiffuseTexture = diffuse;
            mDiffuseUnit = diffuseUnit;
            mSpecularTexture = specular;
            mSpecularUnit = specularUnit;
        }

        /// @brief 공유 템플릿을 per-use 가변 인스턴스로 복제 (Unreal MID / Unity renderer.material 패턴).
        MaterialUPtr Clone() const
        {
            return MaterialUPtr(new Material(*this));
        }

        /// @brief Phong shininess 지수 설정 — @c [2, 256] 으로 clamp (기본 @c 32).
        void SetShininess(float v)
        {
            mShininess = (v < 2.0f) ? 2.0f : (v > 256.0f ? 256.0f : v);
        }

        void SetToProgram(const Program *program) const
        {
            int textureCount = 0;
            if (mDiffuseTexture)
            {
                glActiveTexture(GL_TEXTURE0 + mDiffuseUnit);
                Uniforms::SetInt(*program, "material.diffuse", mDiffuseUnit);
                mDiffuseTexture->Bind();
                textureCount++;
            }
            if (mSpecularTexture)
            {
                glActiveTexture(GL_TEXTURE0 + mSpecularUnit);
                Uniforms::SetInt(*program, "material.specular", mDiffuseUnit);
                mSpecularTexture->Bind();
                textureCount++;
            }
            glActiveTexture(GL_TEXTURE0);
            Uniforms::SetFloat(*program, "material.shininess", mShininess);
        }

        // === Getters — 모두 const, 읽기 전용 ===
        const std::string &GetDiffuseTextureName() const { return mDiffuseTextureName; }
        const std::string &GetSpecularTextureName() const { return mSpecularTextureName; }
        const Texture *GetDiffuseTexture() const { return mDiffuseTexture; }
        const Texture *GetSpecularTexture() const { return mSpecularTexture; }
        GLint GetDiffuseUnit() const { return mDiffuseUnit; }
        GLint GetSpecularUnit() const { return mSpecularUnit; }
        float GetShininess() const { return mShininess; }

        /// @brief 디퓨즈 텍스처가 해석된 상태인지 (nullptr 이면 미해석).
        bool IsResolved() const { return mDiffuseTexture != nullptr; }

    private:
        Material() = default;
        std::string mDiffuseTextureName;          ///< 디퓨즈 맵 이름 키 — @c ResourceRegistry::FindTexture 조회
        std::string mSpecularTextureName;         ///< 스페큘러 맵 이름 키 — 비어 있으면 셰이더 측 default 가정
        const Texture *mDiffuseTexture{nullptr};  ///< 이름으로부터 해석된 비소유 텍스처 관찰자
        const Texture *mSpecularTexture{nullptr}; ///< 스페큘러 맵 관찰자
        GLint mDiffuseUnit{0};                    ///< 디퓨즈 sampler2D 에 넣을 텍스처 이미지 유닛 번호
        GLint mSpecularUnit{1};                   ///< 스페큘러 sampler2D 의 유닛 번호
        float mShininess{32.0f};                  ///< Phong shininess — 셰이더 uniform `material.shininess`. 권장 [2,256]
    };
}

#endif // __SJH_MATERIAL_H__
