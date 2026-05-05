#ifndef __SJH_DIAGNOSTICS_GL_LOG_H__
#define __SJH_DIAGNOSTICS_GL_LOG_H__

#pragma once

#include <glad/glad.h>
#include <string_view>

namespace SJH::diagnostics
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
        /// @details 내부적으로 @c glValidateProgram 호출 → @c GL_VALIDATE_STATUS 확인.
        ///          단순 link 성공과 별개로 "지금 이 상태에서 draw 가능한가"를 드라이버에 위임.
        /// @return 검증 성공 시 @c true (실패는 @c warn 레벨 — 치명적은 아니나 의심 신호).
        /// @warning 무거운 호출 — 매 프레임 호출 금지. 개발/디버깅 빌드의 draw 직전에만.
        /// @note 사용 모듈: @c src/program/ 또는 renderer (디버그 빌드 한정)
        static bool CheckProgramValidate(GLuint program, std::string_view tag = {});
    };

    /// @brief GL 런타임 에러 통보 메커니즘 — KHR_debug 콜백 + @c glGetError 폴링 fallback.
    class GLDebug
    {
    public:
        /// @brief 디버그 콜백 등록 시도 — 컨텍스트 + glad 로드 직후 **정확히 한 번** 호출.
        /// @details
        ///  - KHR_debug 가능 → @c glDebugMessageCallback 등록 → 모든 GL 에러가 자동으로 spdlog 출력.
        ///  - 불가능(예: macOS GL 4.1) → no-op + 안내 로그. 이 경우 @c SJH_GL_CHECK 로 보완해야 함.
        /// @note 호출 위치: @c app/main.cpp 또는 @c src/context/ (컨텍스트 초기화 시점)
        static void Init();

        /// @brief @c SJH_GL_CHECK 매크로 내부 구현. **직접 호출 금지** — 매크로를 통해 사용.
        /// @details @c glGetError 는 큐 형태라 한 번에 모두 비워야 함. 본 함수가 그 루프를 담당.
        static bool DrainErrors(const char *expr, const char *file, int line);
    };
}

/// @def SJH_GL_CHECK
/// @brief GL 호출을 감싸 직후 @c glGetError 폴링으로 에러 포착.
/// @details
///  - @c NDEBUG 빌드 → no-op (릴리스 성능 비용 0).
///  - 디버그 빌드 → 호출 직후 에러 큐 검사 + 위치(파일:라인) 함께 출력.
///  - KHR_debug 콜백 미지원 환경(macOS GL 4.1 등)에서 GL 에러 추적의 **안전망**.
/// @par 예시
/// @code
/// SJH_GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));
/// SJH_GL_CHECK(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
/// @endcode
/// @note 사용 모듈: 모든 GL 호출 모듈 — 특히 @c src/buffer/, @c src/layout/, draw 호출부.
#ifdef NDEBUG
    #define SJH_GL_CHECK(x) (x)
#else
    #define SJH_GL_CHECK(x)                                                          \
        do                                                                           \
        {                                                                            \
            (x);                                                                     \
            ::SJH::diagnostics::GLDebug::DrainErrors(#x, __FILE__, __LINE__);        \
        } while (0)
#endif

#endif // __SJH_DIAGNOSTICS_GL_LOG_H__
