/**
 * @file light.h
 * @brief **Placeholder 모듈** — 점광원/방향광 등 광원 객체 캡슐화 예정.
 *
 * @details 현재 라이팅 상태는 @ref SJH::Context 가 직접 보유 (
 *          @c mLightPos / @c mLightColor / @c mAmbientStrength / @c mSpecularStrength /
 *          @c mSpecularShininess). 광원이 2개 이상으로 늘어나거나 광원 타입(점/방향/스포트)을
 *          분기해야 할 때 본 파일에 @c Light / @c PointLight / @c DirectionalLight 클래스를 신설.
 * @note    파일이 비어 있는 동안에도 @c src/object/CMakeLists.txt 의 @c sjhopengl_object
 *          INTERFACE 타겟을 통해 include path 만 노출됨.
 */

#ifndef __SJH_LIGHT_H__
#define __SJH_LIGHT_H__
#include <glm/glm.hpp>

// light parameter
class Light
{
public :
    /// @brief 점 광원 월드 좌표. ImGui DragFloat3 위젯이 갱신, 셰이더 uniform `lightPos` 로 전달.
    glm::vec3 mPos{glm::vec3(3.0f, 3.0f, 3.0f)};

    /// @brief 점 광원 색상 (RGB, 0~1). 셰이더 uniform `lightColor`.
    glm::vec3 mAmbient{glm::vec3(0.1f, 0.1f, 0.1f)};

    glm::vec3 mDiffuse{glm::vec3(0.5f, 0.5f, 0.5f)};


    glm::vec3 mSpecular{glm::vec3(1.0f, 1.0f, 1.0f)};
};

#endif // __SJH_LIGHT_H__
