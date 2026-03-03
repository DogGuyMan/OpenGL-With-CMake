#pragma once
#include <string_view>
#include <glad/glad.h>   // glad는 반드시 glfw3보다 먼저 포함되어야 함
#include <GLFW/glfw3.h>

namespace glfw_utils {

/// GLFW action 값(GLFW_PRESS / GLFW_RELEASE / GLFW_REPEAT)을 문자열로 변환
constexpr std::string_view ActionToString(int action) noexcept
{
    switch (action) {
        case GLFW_PRESS:   return "Pressed";
        case GLFW_RELEASE: return "Released";
        case GLFW_REPEAT:  return "Repeat";
        default:           return "Unknown";
    }
}

/// GLFW modifier 비트에서 Control 키 표시 문자 반환
constexpr std::string_view ModCtrl (int mods) noexcept { return (mods & GLFW_MOD_CONTROL) ? "C" : "-"; }
/// GLFW modifier 비트에서 Shift   키 표시 문자 반환
constexpr std::string_view ModShift(int mods) noexcept { return (mods & GLFW_MOD_SHIFT)   ? "S" : "-"; }
/// GLFW modifier 비트에서 Alt     키 표시 문자 반환
constexpr std::string_view ModAlt  (int mods) noexcept { return (mods & GLFW_MOD_ALT)     ? "A" : "-"; }

} // namespace glfw_utils
