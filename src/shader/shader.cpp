#include "context/context.h"
#include "diagnostics/gl_log.h"
#include <memory>

namespace SJH
{
    ShaderUPtr Shader::CreateFromFile(const std::string &filename, GLenum shader_type)
    {
        // 생성자를 Private로 하였다고 해서 내부에서 호출 못하는것은 아니네?
        auto shader = std::unique_ptr<Shader>(new Shader());
        if (!shader->TryLoadFile(filename, shader_type))
            return nullptr;
        // UPtr를 Move 소유권 이전.
        return std::move(shader);
    }

    Shader::~Shader()
    {
        if(mShaderAddr != 0)
            glDeleteShader(mShaderAddr);
    }

    bool Shader::TryLoadFile(const std::string &filename, GLenum shader_type)
    {
        auto result = LoadTextFile(filename);
        if (!result.has_value())
            return false;

        auto &code = result.value();
        const char *codePtr = code.c_str();
        GLint codeLength = (GLint)code.length();

        // OpenGL shader object 생성
        mShaderAddr = glCreateShader(shader_type);

        // shader에 소스 코드 설정
        glShaderSource(mShaderAddr, 1, &codePtr, &codeLength);

        // 쉐이더 컴파일
        glCompileShader(mShaderAddr);
        bool isSuccess = diagnostics::GLObjectLog::CheckShaderCompile(mShaderAddr, filename);
        return isSuccess;
    }
}
