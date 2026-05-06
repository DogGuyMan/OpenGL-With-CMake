/**
 * @file program_uniforms.h
 * @brief 프로그램별 uniform location 캐시 + setter family.
 *
 * @details
 *  ### 책임
 *  - **location 캐시** — 매 호출 @c glGetUniformLocation 반복을 1회로 압축.
 *  - **uniform 값 setter** — @c glUniformXxx 호출 (본 클래스의 본질적 기능).
 */

#ifndef __SJH_PROGRAM_UNIFORMS_H__
#define __SJH_PROGRAM_UNIFORMS_H__

#pragma once

#include <glad/glad.h>
#include <string>
#include <unordered_map>

namespace SJH
{
    class ProgramUniforms
    {
    public:
        /// @brief 생성자에서 *eager build* — 모든 active uniform 의 location + type 을 미리 캐싱.
        /// @param program 대상 program 핸들. 본 인스턴스 수명 동안 유효해야 함 (호출자 책임).
        explicit ProgramUniforms(GLuint program);

        // setter family — uniform 값 즉시 GL 전달. 캐시 히트 시 lookup 비용 0.
        // 누락 / 타입 불일치 시 diagnostics::UniformDiagnostics 가 첫 호출 1회 warn.
        void SetMat4 (const char *name, const float *mat4); ///< 기대 타입: GL_FLOAT_MAT4
        void SetVec4 (const char *name, const float *v4);   ///< GL_FLOAT_VEC4
        void SetVec3 (const char *name, const float *v3);   ///< GL_FLOAT_VEC3
        void SetVec2 (const char *name, const float *v2);   ///< GL_FLOAT_VEC2
        void SetFloat(const char *name, float v);           ///< GL_FLOAT
        void SetInt  (const char *name, int v);             ///< GL_INT / GL_SAMPLER_2D 등 (타입 검증 생략)

        /// @brief 캐시된 location 반환. 미존재면 -1 (+ 첫 호출 시 diagnostics 가 warn).
        /// @details 배열 원소(@c "arr[3]") 처럼 eager 캐시에 없는 이름도 lazy lookup 으로 보강.
        GLint Get(const char *name);

    private:
        struct Entry
        {
            GLint  location{-1}; ///< -1 이면 미존재
            GLenum type{0};      ///< 0 이면 active 정보 없음 (lazy 보강 케이스)
        };

        /// @brief 순수 캐시 lookup — warn 등 진단 부수효과 없음. 진단은 setter 가 위임.
        const Entry &Lookup(const char *name);

        GLuint mProgram;
        std::unordered_map<std::string, Entry> mEntries;
    };
}

#endif // __SJH_PROGRAM_UNIFORMS_H__
