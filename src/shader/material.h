/**
 * @file material.h
 * @brief
 * @details
 * @note
 */
#ifndef __SJH_MATERIAL_H__
#define __SJH_MATERIAL_H__
#include <glm/glm.hpp>

// material parameter
class Material
{
public:
    glm::vec3 mAmbient{glm::vec3(1.0f, 0.5f, 0.3f)};
    glm::vec3 mDiffuse{glm::vec3(1.0f, 0.5f, 0.3f)};
    glm::vec3 mSpecular{glm::vec3(0.5f, 0.5f, 0.5f)};
    float mShininess{32.0f};
};

#endif // __SJH_MATERIAL_H__
