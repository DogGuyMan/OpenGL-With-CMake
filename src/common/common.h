#ifndef __SJH_COMMON_H__
#define __SJH_COMMON_H__

#pragma once

#include <memory>
#include <string>
#include <optional>

#define CLASS_PTR(klassName) \
class klassName; \
using klassName ## UPtr = std::unique_ptr<klassName>; \
using klassName ## Ptr = std::shared_ptr<klassName>; \
using klassName ## WPtr = std::weak_ptr<klassName>;

namespace SJH {
    std::optional<std::string> LoadTextFile(const std::string& filename);
}

#endif //__SJH_COMMON_H__
