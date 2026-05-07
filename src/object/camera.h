#ifndef __SJH_CAMERA_H__
#define __SJH_CAMERA_H__

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace SJH
{
    class Camera
    {
    public:
        glm::vec3 mCameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
        glm::vec3 mCameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 mCameraUp = glm::vec3(0.0f, 1.0f, 1.0f);

        bool mCameraControl{false};
        float mCameraPitch{0.0f};
        float mCameraYaw{0.0f};

        float Fov = 60.0f;
        float Aspect = 1.0f;
        float NearPlane = 0.1f;
        float FarPlane = 1000.0f;

        glm::mat4 GetViewMatrix() const
        {
            return glm::lookAt(mCameraPos, mCameraPos + mCameraFront, mCameraUp);
        }

        glm::mat4 GetProjMatrix() const
        {
            return glm::perspective(Fov, Aspect, NearPlane, FarPlane);
        }

        glm::mat4 GetProjMatrix(float width, float height) const
        {
            return glm::perspective(glm::radians(45.0f),
                                    width / height, 0.01f, 20.0f);
        }
    };
} // namespace Engine::Camera

#endif //__SJH_CAMERA_H__
