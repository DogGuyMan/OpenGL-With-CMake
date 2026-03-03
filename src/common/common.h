#ifndef __SJH_COMMON_H__
#define __SJH_COMMON_H__

#pragma once

#include <memory>
#include <string>
#include <optional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace SJH {
    std::optional<std::string> LoadTextFile(const std::string& filename);
}

#endif //__SJH_COMMON_H__
