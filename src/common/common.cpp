#include "common.h"
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>

namespace SJH {
    std::optional<std::string> LoadTextFile(const std::string& filename) {
        // cpp 스타일의 파일 로팅 방식이다.
        std::ifstream fin(filename);
        if(!fin.is_open()) {
            spdlog::error("failed to open file: {}", filename);
            return {};
        }
        std::stringstream text; // ???
        text << fin.rdbuf(); // ???
        return text.str(); // ???
    }
}
