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
    std::string mDiffuseTextureName;
    std::string mSpecularTextureName;
    float mShininess{32.0f};
};

#endif // __SJH_MATERIAL_H__
