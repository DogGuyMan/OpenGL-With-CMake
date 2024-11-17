#ifndef __SHADER_H__
#define __SHADER_H__

#pragma once

#include "common.h"

CLASS_PTR(Shader);
class Shader {
public : 
    // 생성자 대신에 사용할 팩토리 함수
    static ShaderUPtr CreateFromFile(const std::string& filename, GLenum shaderType);
    
    // 밖에서는 소멸자가 호출 되도록 한다.
    ~Shader();
    
    // 오직 ReadOnly하게 바깥쪽으로 값들이 최대한 노출되는것을 막음
    uint32_t Get() const {return mShader;} 
private :
    // 기본 생성자 지우기 생성 함수를 팩토리에 위임하기 위함 따라서 바깥쪽에서 실행하도록 함
    Shader() {} 
    
    // Try.. 구문이랑 비슷하다 생성에 실패할 경우 false를 리턴해서 에러 핸들링의 책임을 호출자에게 넘김
    bool LoadFile(const std::string& filename, GLenum shaderType);
    
    // OpenGL이 리턴해줄 Shader의 ID
    uint32_t mShader { 0 }; 
};

#endif // __SHADER_H__