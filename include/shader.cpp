#include "shader.h"

ShaderUPtr Shader::CreateFromFile(const std::string &filename, GLenum shaderType)
{
    // 여기에서 new Shader을 호출 할 수 있는 이유는
    // 아직 여기는 Shader Class 내부이기 떄문에 접근이 가능한것이다.
    // private라도, 자기 자신의 멤버는 접근 가능한것처럼
    auto shader = std::unique_ptr<Shader>(new Shader());
    if (!shader->LoadFile(filename, shaderType))
        return nullptr;
    return std::move(shader); // 내부에서 동적 생성된 개체의 소유권을 호출자에게 넘긴다.
}

Shader::~Shader()
{
    if (mShader)
    {
        glDeleteShader(mShader);
    }
}

bool Shader::LoadFile(const std::string &filename, GLenum shaderType)
{
    std::optional<std::string> result = LoadTextFile(filename);
    // optional의 값이 있냐 없냐를 확인한다.
    if (!result.has_value())
        return false;

    const std::string &code = result.value();
    const char *codePtr = code.c_str();

    // typedef char int8_t
    // typedef short int16_t
    // typedef int int32_t
    // typedef long long int64_t
    int32_t codeLength = (int32_t)code.length();

    // 쉐이더 컴파일 후 ID 저장
    mShader = glCreateShader(shaderType);
    // 여려개의 코드 를 넣을 수 있음
    // <코드 갯수>, <각각의 코드 갯수에 해당하는 코드 포인터 배열>, <각 코드마다 길이의 배열>
    glShaderSource(mShader, 1, (const GLchar *const *)&codePtr, &codeLength);
    glCompileShader(mShader);

    // check shader code compile error
    int success = 0;

    // iv = integer vector success에 집어넣어야 하는 타입을 포인트로 &success를 기입ㄴ
    glGetShaderiv(mShader, GL_COMPILE_STATUS, &success);
    if (success == 0)
    {
        char infoLog[1024];
        glGetShaderInfoLog(mShader, 1024, nullptr, infoLog);
        SPDLOG_ERROR("failed to compile shader : \"{}\"", filename);
        SPDLOG_ERROR("reason : {}", infoLog);
        return false;
    }
    return true;
}