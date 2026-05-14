#ifndef __SJH_TRANSFORM_H__
#define __SJH_TRANSFORM_H__

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace SJH
{
    class INameTagInterface
    {
    protected:
        INameTagInterface() = default;

    public:
        virtual ~INameTagInterface() = default;
        INameTagInterface(const INameTagInterface &) = delete;
        INameTagInterface operator=(const INameTagInterface &) = delete;
        INameTagInterface(INameTagInterface &&) = delete;
        INameTagInterface operator=(INameTagInterface &) = delete;
        virtual const std::string &GetName() const = 0;
    };

    class Transform : public INameTagInterface
    {
    public:
        std::string Name;

        glm::vec3 Translate = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 EulerRot = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 Scale = glm::vec3(1.0f, 1.0f, 1.0f);

        Transform *Parent = nullptr;
        std::unordered_map<std::string, Transform *> Children;

        virtual const std::string &GetName() const override
        {
            return Name;
        }

        glm::mat4 GetModelMatrix() const
        {
            glm::mat4 local =
                glm::translate(glm::mat4(1.0), Translate) *
                glm::rotate(glm::mat4(1.0f), glm::radians(EulerRot[2]), glm::vec3(0.0f, 0.0f, 1.0f)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(EulerRot[1]), glm::vec3(0.0f, 1.0f, 0.0f)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(EulerRot[0]), glm::vec3(1.0f, 0.0f, 0.0f)) *
                glm::scale(glm::mat4(1.0f), Scale);
            if (Parent == nullptr)
                return local;
            return Parent->GetModelMatrix() * local;
        }
    };

    class UVTransform
    {
    public:
        glm::vec2 Offset{0.0f, 0.0f};
        glm::vec2 Scale{1.0f, 1.0f};
        float RotationDeg = 0.0f;
    };
}
#endif //__SJH_TRANSFORM_H__
