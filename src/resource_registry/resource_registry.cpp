/**
 * @file resource_registry.cpp
 * @brief Create* — 캐시-미스 경로에서 새 자원을 생성.
 *        Find*   — 캐시-히트 경로에서 기존 인스턴스를 즉시 반환.
 *
 * @details emplace 결과의 iterator 로 raw 포인터를 꺼내 반환 — 매니저 보관 인스턴스를 가리키므로
 *          호출자에게 노출되는 lifetime 은 매니저 자신의 lifetime 과 동일하다.
 *          Image 는 스코프 한정 — GPU 업로드 후 Create* 스택 프레임을 벗어나면 즉시 소멸.
 */
#include "resource_registry.h"
#include <memory>
#include <spdlog/spdlog.h>

namespace SJH
{
    ResourceRegistryUPtr ResourceRegistry::Create()
    {
        auto resourceManager = std::unique_ptr<ResourceRegistry>(new ResourceRegistry());
        return std::move(resourceManager);
    }

    ResourceRegistry::~ResourceRegistry()
    {
        Clear();
    }

    Texture *ResourceRegistry::CreateTexture(const std::string &key, const Image *image)
    {
        if (textures.find(key) != textures.end())
        {
            spdlog::warn("CreateTexture: 키 '{}' 가 이미 존재 — Find 를 먼저 호출하라", key);
            return nullptr;
        }
        auto texture = Texture::CreateTexture(image);
        if (texture == nullptr)
        {
            spdlog::error("CreateTexture: GPU 텍스처 생성 실패 — key '{}'", key);
            return nullptr;
        }
        auto insertedIt = textures.emplace(key, std::move(texture)).first;
        return insertedIt->second.get();
    }

    Texture *ResourceRegistry::FindTexture(const std::string &key)
    {
        auto it = textures.find(key);
        return (it != textures.end()) ? it->second.get() : nullptr;
    }

    Material *ResourceRegistry::CreateMaterial(const std::string &key, const std::string &filepath)
    {
        if (materials.find(key) != materials.end() || textures.find(key) != textures.end())
        {
            spdlog::warn("CreateMaterial: 키 '{}' 가 이미 존재 — Find 를 먼저 호출하라", key);
            return nullptr;
        }

        auto image = Image::Load(key, filepath);   // 스코프 한정 — 이 함수 끝에서 소멸
        if (image == nullptr)
            return nullptr;

        auto texture = Texture::CreateTexture(image.get());
        if (texture == nullptr)
        {
            spdlog::error("CreateMaterial: 텍스처 생성 실패 — key '{}'", key);
            return nullptr;
        }
        // 텍스처를 registry 캐시에 보관 — Material 핸들의 lifetime owner (co-location).
        // 진입부 검사로 키 충돌이 없음이 보장되므로 emplace 는 항상 성공.
        auto texIt = textures.emplace(key, std::move(texture)).first;
        const Texture *diffusePtr = texIt->second.get();

        auto material = Material::Create();
        material->SetDiffuseTextureName(key);
        material->SetResolvedTextures(diffusePtr, /*diffuseUnit*/ 0,
                                      /*specular*/ nullptr, /*specularUnit*/ 1);

        auto insertedIt = materials.emplace(key, std::move(material)).first;
        return insertedIt->second.get();
    }

    Material *ResourceRegistry::FindMaterial(const std::string &key)
    {
        auto it = materials.find(key);
        return (it != materials.end()) ? it->second.get() : nullptr;
    }

    Model *ResourceRegistry::CreateModel(const std::string &key, const std::string &filename)
    {
        if (models.find(key) != models.end())
        {
            spdlog::warn("CreateModel: 키 '{}' 가 이미 존재 — Find 를 먼저 호출하라", key);
            return nullptr;
        }
        auto model = Model::Load(filename);
        if (model == nullptr)
        {
            spdlog::error("CreateModel: 모델 로드 실패 — key '{}', file '{}'", key, filename);
            return nullptr;
        }
        auto insertedIt = models.emplace(key, std::move(model)).first;
        return insertedIt->second.get();
    }

    Model *ResourceRegistry::FindModel(const std::string &key)
    {
        auto it = models.find(key);
        return (it != models.end()) ? it->second.get() : nullptr;
    }

    void ResourceRegistry::Clear()
    {
        textures.clear();
        materials.clear();
        models.clear();
    }

}
