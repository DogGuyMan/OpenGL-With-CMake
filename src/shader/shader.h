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
        // 팩토리 함수의 동기 (CreateXXX -> ShaderUPtr)
        /*
        1. 예외 없는 실패 처리
            * 성자는 실패를 신호할 방법이 없음
            * 팩토리는 nullptr 반환으로 실패를 깔끔히 표현 가능.

        2. RAII 소유권 강제 — 두 메커니즘이 협력
            * 생성자 private  -> 직접 생성 차단 (Shader s; / new Shader 모두 불가).
            * 반환 타입 UPtr -> 인스턴스 획득 시점에 unique_ptr 소유권 자동 채택.

        3. 클래스 불변식 — 외부에 노출되는 Shader 는 항상 유효한 GL 핸들 보유
            - 실제 메커니즘:
              1. 팩토리 내부에서 Shader 빈 객체 *완전 생성* (mShaderAddr=0)
              2. TryLoadFile 로 GL 자원 획득 시도
              3.
                성공 -> UPtr 소유권 외부로 이전
                실패 -> 임시 UPtr 스코프 종료 -> 즉시 파괴 + nullptr 반환
            * 따라서 호출자가 받는 Shader 는 "GL 자원 획득까지 완료된" 인스턴스만.
        */
        static ShaderUPtr CreateFromFile(const std::string &filename, GLenum shader_type);

        ~Shader();
        GLuint GetShaderAddr() const { return mShaderAddr; }

    private:
        // 생성자 private
        //  -> CreateXXX로 팩토리 함수를 만들어서 Shader 인스턴스 막기
        Shader() = default;
        bool TryLoadFile(const std::string &filename, GLenum shader_type);
        GLuint mShaderAddr{0};
    };
}
#endif // __SHADER_H__
