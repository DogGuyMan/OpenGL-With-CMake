#ifndef __MESH_H__
#define __MESH_H__

#include "buffer/buffer.h"
#include "common/common.h"
#include "layout/vertex_layout.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace SJH
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };

    CLASS_PTR(Mesh);
    class Mesh
    {
    public:
        static MeshUPtr Create(const std::vector<Vertex> &vertices,const std::vector<GLuint> &indices,GLuint primitiveType);
        static MeshUPtr CreateBox();

        const VertexLayout *GetVertexLayout() const
        {
            return mVertexLayout.get();
        }
        BufferPtr GetVertexBuffer() const { return mVertexBuffer; }
        BufferPtr GetIndexBuffer() const { return mIndexBuffer; }

        void Draw() const;

    private:
        Mesh() {}
        void Init(const std::vector<Vertex> &vertices,const std::vector<GLuint> &indices,GLuint primitiveType);

        GLuint mPrimitiveType{GL_TRIANGLES};
        VertexLayoutUPtr mVertexLayout;
        BufferPtr mVertexBuffer;
        BufferPtr mIndexBuffer;
    };

}

#endif // __MESH_H__
