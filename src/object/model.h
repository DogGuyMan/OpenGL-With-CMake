#ifndef __MODEL_H__
#define __MODEL_H__

#include "common/common.h"
#include "object/mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace SJH
{
    CLASS_PTR(Model);
    class Model
    {
    public:
        static ModelUPtr Load(const std::string &filename);

        int GetMeshCount() const { return (int)mMeshes.size(); }
        MeshPtr GetMesh(int index) const { return mMeshes[index]; }
        void Draw() const { for (auto &mesh : mMeshes) mesh->Draw(); };

    private:
        Model() {}
        bool LoadByAssimp(const std::string &filename);
        void ProcessMesh(aiMesh *mesh, const aiScene *scene);
        void ProcessNode(aiNode *node, const aiScene *scene);

        // Mesh Shadered Ptr 존재.
        std::vector<MeshPtr> mMeshes;
    };

} // namespace

#endif // __MODEL_H__
