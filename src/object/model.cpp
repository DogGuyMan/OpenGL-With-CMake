#include "model.h"
#include "resource_registry/texture.h"
#include "shader/material.h"
#include <spdlog/spdlog.h>

namespace SJH
{
    ModelUPtr Model::Load(const std::string &filename)
    {
        auto model = ModelUPtr(new Model());
        if (!model->LoadByAssimp(filename))
            return nullptr;
        return std::move(model);
    }

    // scene->mRootNode부터 재귀적으로 처리
    bool Model::LoadByAssimp(const std::string &filename)
    {
        Assimp::Importer importer;
        auto scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            spdlog::error("failed to load model: {}", filename);
            return false;
        }

        auto dirname = filename.substr(0, filename.find_last_of("/"));
        auto LoadTexture = [&](aiMaterial *material, aiTextureType type) -> Texture*
        {
            if (material->GetTextureCount(type) <= 0)
                return nullptr;
            aiString filepath;
            material->GetTexture(type, 0, &filepath); // 🛠 type 인자 사용 (이전 코드는 DIFFUSE 하드코딩 버그)
            const auto fullPath = fmt::format("{}/{}", dirname, filepath.C_Str());
            auto image = Image::Load(filepath.C_Str(), fullPath); // 🛠 새 API: (image_name, filepath)
            if (!image)
                return nullptr;
            auto tex = Texture::CreateTexture(image.get());   // TextureUPtr
            if (!tex)
                return nullptr;
            mTextures.push_back(std::move(tex));              // Model 이 lifetime owner
            return mTextures.back().get();                    // 비소유 관찰자 반환
        };

        for (uint32_t i = 0; i < scene->mNumMaterials; i++)
        {
            auto aiMat = scene->mMaterials[i];
            auto glMaterial = Material::Create();

            const auto diffuse  = LoadTexture(aiMat, aiTextureType_DIFFUSE);
            const auto specular = LoadTexture(aiMat, aiTextureType_SPECULAR);

            // 새 Material API — 이름은 비워두고 *해석된 관찰자 + sampler 유닛* 만 설정.
            glMaterial->SetResolvedTextures(
                /*diffuse */ diffuse,  /*diffuseUnit*/ 0,
                /*specular*/ specular, /*specularUnit*/ 1);

            mMaterials.push_back(std::move(glMaterial)); // 🛠 m_materials → mMaterials
        }

        ProcessNode(scene->mRootNode, scene);
        return true;
    }

    // Node는 Tree 형태로 구성되어 있음.
    void Model::ProcessNode(aiNode *node, const aiScene *scene)
    {
        // 현 계층 Sibling 처리
        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            auto meshIndex = node->mMeshes[i];
            auto mesh = scene->mMeshes[meshIndex];
            ProcessMesh(mesh, scene);
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++)
        {
            // 재귀적으로 호출중.
            ProcessNode(node->mChildren[i], scene);
        }
    }

    void Model::ProcessMesh(aiMesh *mesh, const aiScene *scene)
    {
        spdlog::info("process mesh: {}, #vert: {}, #face: {}",
                     mesh->mName.C_Str(), mesh->mNumVertices, mesh->mNumFaces);

        // 1. Vertex 정보 수집
        std::vector<Vertex> vertices;
        vertices.resize(mesh->mNumVertices);
        for (uint32_t i = 0; i < mesh->mNumVertices; i++)
        {
            auto &v = vertices[i];
            v.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            v.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            v.texCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }

        // 2. Indices 정보 수집
        std::vector<uint32_t> indices;
        indices.resize(mesh->mNumFaces * 3);
        for (uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
            indices[3 * i] = mesh->mFaces[i].mIndices[0];
            indices[3 * i + 1] = mesh->mFaces[i].mIndices[1];
            indices[3 * i + 2] = mesh->mFaces[i].mIndices[2];
        }

        // 우리가 만들었던 Mesh 코드 호출
        auto glMesh = Mesh::Create(vertices, indices, GL_TRIANGLES);
        Material* mat = nullptr;
        if (mesh->mMaterialIndex < mMaterials.size()) // 🛠 m_materials → mMaterials, unsigned 범위 안전
            mat = mMaterials[mesh->mMaterialIndex].get();
        mRenderUnit.push_back({std::move(glMesh), mat});
    }
}
