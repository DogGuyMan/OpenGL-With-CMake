#ifndef __MODEL_H__
#define __MODEL_H__

#include "common/common.h"
#include "object/mesh.h"
#include "resource_registry/texture.h"   // TextureUPtr — 모델이 보유하는 텍스처 lifetime
#include "shader/material.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace SJH
{
    struct RenderUnit {
        MeshUPtr  mesh;                ///< 1 RenderUnit : 1 Mesh — RenderUnit 이 유일 소유
        Material* material{nullptr};   ///< 비소유 관찰자 — owner 는 Model::mMaterials
    };

    CLASS_PTR(Model);
    class Model
    {
    public:
        static ModelUPtr Load(const std::string &filename);

        int GetMeshCount() const { return (int)mRenderUnit.size(); }
        /// @brief index 번째 메시의 비소유 관찰자. Model 보다 오래 보관 금지 — owner 는 RenderUnit.
        Mesh *GetMesh(int index) const { return mRenderUnit[index].mesh.get(); }
        void Draw() const { for (auto &unit : mRenderUnit) unit.mesh->Draw(); };

    private:
        Model() = default;
        bool LoadByAssimp(const std::string &filename);
        void ProcessMesh(aiMesh *mesh, const aiScene *scene);
        void ProcessNode(aiNode *node, const aiScene *scene);

        std::vector<TextureUPtr>   mTextures;     ///< 로드한 텍스처 — Material 핸들의 lifetime owner. 관찰자보다 먼저 선언(나중 소멸).
        std::vector<MaterialUPtr>  mMaterials;    ///< Material 인스턴스 — 인덱스는 assimp mMaterialIndex.
        std::vector<RenderUnit>    mRenderUnit;   ///< 메시 목록 (Material 관찰자 보유).
    };

} // namespace

#endif // __MODEL_H__
