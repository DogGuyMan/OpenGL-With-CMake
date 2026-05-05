#ifndef __SHADER_H__
#define __SHADER_H__

#pragma once

#include <glad/glad.h>
#include "common/common.h"

namespace SJH
{
    CLASS_PTR(Shader)
    class Shader
    {
    public:
        static ShaderUPtr CreateFromFile(const std::string &filename, GLenum shader_type);

        ~Shader();
        GLuint Get() const { return mShader; }

    private:
        // 생성자 private
        //  -> CreateXXX로 팩토리 함수를 만들어서 Shader 인스턴스 막기
        Shader() = default;
        bool TryLoadFile(const std::string &filename, GLenum shader_type);
        GLuint mShader{0};
    };
}
#endif // __SHADER_H__
