#include "shader.h"
#include <memory>
#include <spdlog/spdlog.h>

namespace SJH
{
    ShaderUPtr Shader::CreateFromFile(const std::string &filename, GLenum shader_type)
    {
        // 생성자를 Private로 하였다고 해서 내부에서 호출 못하는것은 아니네?
        auto tempShader = std::unique_ptr<Shader>(new Shader());
        if (!tempShader->TryLoadFile(filename, shader_type))
            return nullptr;
        // UPtr를 Move 소유권 이전.
        return std::move(tempShader);
    }

    Shader::~Shader()
    {
    }

    bool Shader::TryLoadFile(const std::string &filename, GLenum shader_type)
    {
        auto result = LoadTextFile(filename);
        if (!result.has_value())
            return false;

        auto &code = result.value();
        const char *codePtr = code.c_str();
        GLint codeLength = (GLint)code.length();

        // create and compile shader
        mShader = glCreateShader(shader_type);
        glShaderSource(mShader, 1, &codePtr, &codeLength);
        glCompileShader(mShader);

        int success = 0;
        glGetShaderiv(mShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[1024];
            glGetShaderInfoLog(mShader, 1024, nullptr, infoLog);
            SPDLOG_ERROR("failed to compile shader: \"{}\"", filename);
            SPDLOG_ERROR("reason: {}", infoLog);
            return false;
        }
        return true;
    }
}
