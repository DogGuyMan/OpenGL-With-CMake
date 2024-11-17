#ifndef __COMMON_H__
#define __COMMON_H__

#pragma once

#include <memory>
#include <string>
#include <optional>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <spdlog/spdlog.h>

// 매크로 함수 \로 여러줄로 작성 가능
// ##의 의미는 klassName과 Ptr의 변수를 붙여달라는 의미
// 앞으로 이렇게 클래스 디자인을 할것.
#define CLASS_PTR(klassName) \ 
class klassName; \
using klassName ## UPtr = std::unique_ptr<klassName>; \
using klassName ## Ptr = std::shared_ptr<klassName>; \
using klassName ## WPtr = std::weak_ptr<klassName>;

/***************************

CLASS_PTR(Shader);
class Shader;
using ShaderUPtr = std::unique_ptr<Shader>;
using ShaderPtr = std::shared_ptr<Shader>;
using ShaderWPtr = std::weak_ptr<Shader>;

***************************/

// std::string* LoadTextFile(const std::string& filename); 대신 사용한다
std::optional<std::string> LoadTextFile(const std::string& filename);

#endif // __COMMON_H__