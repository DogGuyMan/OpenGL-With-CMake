#ifndef __GLFW_INPUT_UTILS_H__
#define __GLFW_INPUT_UTILS_H__

#pragma once

// glad가 GLFW보다 먼저 와야 함 — GLFW가 시스템 <GL/gl.h>를 끌어와서
// glad와 OpenGL 심볼이 이중 정의되는 것을 방지하는 *순서 가드*.
// 이 헤더 자체는 glad 타입을 안 쓰지만, 순서 보장을 위해 명시적으로 포함.
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string_view>
namespace glfw_utils
{

    /// GLFW action 값(GLFW_PRESS / GLFW_RELEASE / GLFW_REPEAT)을 문자열로 변환
    constexpr std::string_view ActionToString(int action) noexcept
    {
        switch (action)
        {
        case GLFW_PRESS:
            return "Pressed";
        case GLFW_RELEASE:
            return "Released";
        case GLFW_REPEAT:
            return "Repeat";
        default:
            return "Unknown";
        }
    }

    /// GLFW modifier 비트에서 Control 키 표시 문자 반환
    constexpr std::string_view ModCtrl(int mods) noexcept { return (mods & GLFW_MOD_CONTROL) ? "C" : "-"; }
    /// GLFW modifier 비트에서 Shift   키 표시 문자 반환
    constexpr std::string_view ModShift(int mods) noexcept { return (mods & GLFW_MOD_SHIFT) ? "S" : "-"; }
    /// GLFW modifier 비트에서 Alt     키 표시 문자 반환
    constexpr std::string_view ModAlt(int mods) noexcept { return (mods & GLFW_MOD_ALT) ? "A" : "-"; }

} // namespace glfw_utils

#endif// __GLFW_INPUT_UTILS_H__
