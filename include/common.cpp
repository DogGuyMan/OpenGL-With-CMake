#include "common.h"
#include <fstream>
#include <sstream>

std::optional<std::string> LoadTextFile(const std::string &filename)
{
    /*
     * cpp 스타일의 파일 로팅 방식이다.
     */
    std::ifstream fin(filename);
    if (!fin.is_open())
    {
        SPDLOG_ERROR("failed to open file: {}", filename);
        return {}; // 옵셔널 타입이 아무것도 없으면 {} 로 리턴 가능하다.
    }

    std::stringstream text; // ???
    text << fin.rdbuf();    // ???
    return text.str();      // ???
}