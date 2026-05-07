/**
 * @file resource_management.cpp
 * @brief 캐시-미스 경로에서 새 자원을 생성, 캐시-히트 경로에서 기존 인스턴스를 즉시 반환.
 * @details emplace 결과의 iterator 로 raw 포인터를 꺼내 반환 — 매니저 보관 인스턴스를 가리키므로
 *          호출자에게 노출되는 lifetime 은 매니저 자신의 lifetime 과 동일하다.
 */
#include "resource_management.h"
#include <memory>

namespace SJH
{
    ResourceManagementUPtr ResourceManagement::CreateRM()
    {
        auto resourceManager = std::unique_ptr<ResourceManagement>(new ResourceManagement());
        return std::move(resourceManager);
    }

    ResourceManagement::~ResourceManagement()
    {
        Clear();
    }

    Image *ResourceManagement::LoadImage(const std::string &image_name, const std::string &filepath)
    {
        auto it = images.find(image_name);
        if (it != images.end())
            return it->second.get();

        auto image = Image::Load(image_name, filepath);
        if (image == nullptr)
            return nullptr;

        auto [intertedIt, result] = images.emplace(image_name, std::move(image));
        return intertedIt->second.get();
    }

    Texture *ResourceManagement::LoadTextureWithName(const std::string &image_name)
    {
        auto it = textures.find(image_name);
        if (it != textures.end())
            return it->second.get();
        return nullptr;
    }

    Texture *ResourceManagement::LoadTextureFromImage(const Image *image)
    {
        auto it = textures.find(image->GetImageName());
        if (it != textures.end())
            return it->second.get();

        auto texture = Texture::CreateTexture(image);
        if (texture == nullptr)
            return nullptr;

        auto [intertedIt, result] = textures.emplace(image->GetImageName(), std::move(texture));
        return intertedIt->second.get();
    }
    void ResourceManagement::Clear()
    {
        textures.clear();
        images.clear();
    }

}
