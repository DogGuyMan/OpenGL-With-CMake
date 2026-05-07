#ifndef __SJH_RESOURCE_MANAGEMENT_H__
#define __SJH_RESOURCE_MANAGEMENT_H__

/**
 * @file resource_management.h
 * @brief Image / Texture 자원의 lifecycle 중앙 관리 — 이름 키 캐시 + 일괄 해제.
 */

#include "image.h"
#include "texture.h"
#include <glad/glad.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace SJH
{
    CLASS_PTR(ResourceManagement)
    /**
     * @brief Image / Texture 자원을 *논리 이름 키*로 캐시하고, 매니저 소멸 시 일괄 해제.
     * @details
     *  - 자원 소유권은 매니저 보유 (@c unique_ptr).
     *  - 반환되는 raw 포인터는 *접근 전용* — 호출자는 매니저 수명 동안만 유효함을 가정.
     *  - 동일 키 재호출 시 캐시 인스턴스 재사용 (재로드 / 재업로드 X).
     */
    class ResourceManagement
    {
    public:
        /**
         * @brief 매니저 인스턴스 팩토리.
         * @return 새 매니저 (단일 소유 @c unique_ptr).
         */
        static ResourceManagementUPtr CreateRM();

        /// @brief 보유 자원 일괄 해제 후 매니저 자체 소멸.
        ~ResourceManagement();

        /**
         * @brief 이미지 파일 로드 + 이름 키 캐시.
         * @details 같은 @p image_name 으로 이미 로드된 이미지가 있으면 기존 인스턴스를 재사용 (재로드 X).
         * @param filepath   디스크 이미지 경로.
         * @param image_name 캐시 키 (논리 이름 — 파일명과 별개).
         * @return 캐시되었거나 새로 로드된 @c Image* (소유권 X). 실패 시 @c nullptr.
         */
        Image *LoadImage(const std::string &image_name, const std::string &filepath);

        /**
         * @brief 이름으로 캐시된 텍스처 *조회* (생성 안 함).
         * @details 함수명에 *Load* 가 들어있으나 실제 동작은 *Lookup* — 새 텍스처 생성은 없음.
         *          신규 생성이 필요하면 @c LoadTextureFromImage 사용.
         * @param image_name 캐시 키.
         * @return 캐시된 텍스처 또는 @c nullptr.
         */
        Texture *LoadTextureWithName(const std::string &image_name);

        /**
         * @brief Image 로부터 GPU 텍스처 생성 + 캐시.
         * @details 캐시 키는 @c image->GetImageName(). 동일 이름의 텍스처가 이미 있으면 기존을 반환.
         * @param image GPU 업로드 소스. 매니저는 @p image 소유권을 가져가지 않음 (호출 동안만 유효하면 됨).
         * @return 생성/캐시된 @c Texture* (소유권 X). 실패 시 @c nullptr.
         */
        Texture *LoadTextureFromImage(const Image *image);

        /// @brief 보유 모든 이미지/텍스처 일괄 해제 (매니저 인스턴스 자체는 유지).
        void Clear();

    private:
        std::unordered_map<std::string, TextureUPtr> textures;   ///< 텍스처 캐시 — 키: 논리 이름.
        std::unordered_map<std::string, ImageUPtr> images;       ///< 이미지 캐시 — 키: 논리 이름.
        ResourceManagement() = default;
    };
}

#endif // __SJH_RESOURCE_MANAGEMENT_H__
