#ifndef __COMMON_H__
#define __COMMON_H__

#pragma once

#include <memory>
#include <string>
#include <optional>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <spdlog/spdlog.h>

// std::string* LoadTextFile(const std::string& filename); 대신 사용한다
std::optional<std::string> LoadTextFile(const std::string& filename);

#endif // __COMMON_H__