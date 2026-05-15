#include "model.h"
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
        mMeshes.push_back(std::move(glMesh));
    }
}
