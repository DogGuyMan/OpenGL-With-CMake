#ifndef __SJH_DIAGNOSTICS_GL_LOG_H__
#define __SJH_DIAGNOSTICS_GL_LOG_H__

#pragma once

#include <glad/glad.h>
#include <initializer_list>
#include <string_view>
#include <vector>

namespace SJH::Diagnostics
{
    /// @brief GL 객체(셰이더/프로그램) 상태 쿼리 + InfoLog 일원화 헬퍼.
    /// @details 컴파일/링크/검증은 비동기 아니지만 별도 status 쿼리가 유일한 검증 수단.
    ///          그 반복 보일러플레이트를 본 클래스에 가둔다.
    class GLObjectLog
    {
    public:
        /// @brief @c glCompileShader 직후 호출 — 실패 시 GLSL 컴파일러 InfoLog 를 error 출력.
        /// @param shader 검사할 셰이더 핸들
        /// @param tag    로그 식별자(예: 셰이더 파일 경로). 비우면 생략.
        /// @return 컴파일 성공 시 @c true.
        /// @note 사용 모듈: @c src/shader/
        static bool CheckShaderCompile(GLuint shader, std::string_view tag = {});

        /// @brief @c glLinkProgram 직후 호출 — 실패하면 uniform/attribute location 이 무효해지므로 필수.
        /// @return 링크 성공 시 @c true.
        /// @note 사용 모듈: @c src/program/ (Program 클래스 빌드 직후)
        static bool CheckProgramLink(GLuint program, std::string_view tag = {});

        /// @brief 현재 GL 상태(VAO/uniform/texture 바인딩 등)에서 프로그램 실행 가능성 검증.
        /// @details 내부적으로 @c glValidateProgram 호출 -> @c GL_VALIDATE_STATUS 확인.
        ///          단순 link 성공과 별개로 "지금 이 상태에서 draw 가능한가"를 드라이버에 위임.
        /// @return 검증 성공 시 @c true (실패는 @c warn 레벨 — 치명적은 아니나 의심 신호).
        /// @warning 무거운 호출 — 매 프레임 호출 금지. 개발/디버깅 빌드의 draw 직전에만.
        /// @note 사용 모듈: @c src/program/ 또는 renderer (디버그 빌드 한정)
        static bool CheckProgramValidate(GLuint program, std::string_view tag = {});

        /// @brief 프로그램에 기대되는 uniform 들이 모두 존재하는지 정적 검증.
        /// @details
        ///  - 각 name 에 대해 @c glGetUniformLocation < 0 검사 -> 누락된 것을 한 번에 spdlog::warn.
        ///  - **같은 program 핸들에 대해 최초 1회만** 실 검사. 이후 호출은 캐시된 bool 즉시 반환,
        ///    로그 출력 없음 -> 매 draw 호출 안에 두어도 부담 0.
        ///  - 누락은 warn 레벨 (error 아님) — 셰이더 옵티마이저가 inactive uniform 제거하는 게 정상이라서.
        /// @return 모든 uniform 존재 시 @c true.
        /// @note 사용 모듈: @c src/context/ 등 program 사용 시점.
        static bool CheckExpectedUniforms(
            GLuint program,
            std::initializer_list<const char *> names,
            std::string_view tag = {});

        /// @brief vertex attribute 동일 — @c glGetAttribLocation < 0 검사 + program 별 1회 캐시.
        /// @note 사용 모듈: @c src/layout/ 또는 program 사용 시점.
        static bool CheckExpectedAttributes(
            GLuint program,
            std::initializer_list<const char *> names,
            std::string_view tag = {});

        /// @brief 캐시 무효화 — @c Program 소멸자에서 호출.
        /// @details 같은 @c GLuint 핸들이 재발급될 때 stale 캐시 결과 방지.
        ///          @c CheckExpectedUniforms / @c CheckExpectedAttributes 의 program 별 1회 캐시를 정리.
        static void InvalidateProgramCache(GLuint program);
    };

    /// @brief GL 런타임 에러 통보 메커니즘 — KHR_debug 콜백 + @c glGetError 폴링 fallback.
    class GLDebug
    {
    public:
        static bool CheckGLGenVertexArrays();
        static bool CheckGLBindVertexArray(const GLuint vao);
        static bool CheckGLGenBuffers(const GLuint vbo);
        static bool CheckGLBindBuffer(const GLuint vbo);
        static bool CheckGLBufferData(const GLint data_size);
        static bool CheckGLEnableVertexAttribArray(GLuint layout_location);
        static bool CheckGLVertexAttribPointer(const std::vector<GLuint>&& strides);
    };
}

#endif // __SJH_DIAGNOSTICS_GL_LOG_H__
