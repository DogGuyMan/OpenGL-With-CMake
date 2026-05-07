/**
 * @file program_uniforms.h
 * @brief @c SJH::Uniforms 자유 함수 family — Program 의 uniform 캐시 + setter (외부 보관).
 *
 * @details
 *  ### 디자인 동기 — 왜 멤버 함수가 아니라 자유 함수인가
 *  -# **책임 분리 (SRP)** — @c Program 의 본질은 *GL program 의 lifetime + link 상태*.
 *     uniform 값 설정은 *그 위에 얹는 별개 관심사* — Program 의 책임이 아님.
 *  -# **확장성 (OCP)** — 새 uniform 타입 (예: @c Mat3, @c IVec2) 추가 시 @c Program 헤더는
 *     *그대로*, 본 namespace 에만 한 줄 추가. 멤버로 두면 매번 Program API 가 비대해짐.
 *  -# **C# extension method 와 동등한 효과** — 클래스 내부를 건드리지 않고 *외부에서*
 *     동작을 덧붙이는 패턴. C++ 에선 *자유 함수 + ADL* 가 그 자연스러운 형태.
 *
 *  ### 캐시 위치 — Program *외부* (TU-local static)
 *  - 캐시는 @c program_uniforms.cpp 의 anonymous namespace 내 정적 자료구조에 보유:
 *    @code{.cpp}
 *      std::unordered_map<GLuint, std::unordered_map<std::string, UniformEntry>>
 *      //                  ^^^ ProgramInstanceId        ^^^^^ name → entry
 *    @endcode
 *  - **friend 선언 불필요** — 자유 함수들이 @c Program::GetProgramAddr() (public) 만 사용.
 *    @c Program 의 헤더는 @c Uniforms 의 존재를 *전혀 모름*.
 *
 *  ### Lifetime — @c BuildCache / @c Forget 짝
 *  - **빌드**: @c Program::Create 가 link 직후 @c BuildCache 호출 (eager).
 *  - **소멸**: @c Program::~Program 이 @c Forget 호출 — 캐시 항목 제거.
 *    GL 이 해당 ID 를 다른 프로그램에 *재할당* 했을 때 stale 캐시 노출 방지.
 *  - 배열 원소 (@c "arr[3]") 같은 비-canonical 이름은 setter 호출 시 lazy 보강.
 *
 *  ### 책임
 *  - **location 캐시 빌드** — @c BuildCache (eager) + lazy 보강.
 *  - **uniform 값 setter** — 6종 (Mat4 / Vec4 / Vec3 / Vec2 / Float / Int).
 *  - **진단 위임** — 누락 / 타입 불일치 시 @c Diagnostics::UniformDiagnostics 가 첫 호출 1회 warn.
 *
 *  ### 사용
 *  @code
 *    auto prog = Program::Create({vs, fs});      // BuildCache 자동 호출됨
 *    Uniforms::SetMat4(*prog, "uModel", data);
 *  @endcode
 */

#ifndef __SJH_PROGRAM_UNIFORMS_H__
#define __SJH_PROGRAM_UNIFORMS_H__

#include <glad/glad.h>

namespace SJH
{
    class Program;   // forward — 본 헤더는 Program 의 정의에 의존하지 않음 (의도된 decoupling).

    namespace Uniforms
    {
        /// @brief 캐시 한 칸 — name → (location, type).
        struct UniformEntry
        {
            GLint  location{-1};   ///< -1 이면 미존재
            GLenum type{0};        ///< 0 이면 active 정보 없음 (lazy 보강 케이스)
        };

        /// @brief Program link 직후 호출 — 모든 active uniform 의 (name, location, type) 캐싱.
        /// @details @c Program::Create 가 자동으로 호출. link 후 셰이더 source 변경 → 다시 link 시 재호출 필요.
        void BuildCache(Program &prog);

        /// @brief Program 소멸 시 호출 — 외부 캐시에서 해당 program ID 의 항목 제거.
        /// @details GL ID 재사용에 의한 stale 노출 방지. @c Program::~Program 이 자동 호출.
        /// @internal 일반 호출자가 직접 부를 일은 없음 — Program 의 destructor 에서만.
        void Forget(GLuint programId);

        // --- setter family — 캐시 히트 시 lookup 비용 0. 미존재 이름은 첫 호출 1회 warn ---
        void SetMat4 (Program &prog, const char *name, const float *mat4); ///< GL_FLOAT_MAT4
        void SetVec4 (Program &prog, const char *name, const float *v4);   ///< GL_FLOAT_VEC4
        void SetVec3 (Program &prog, const char *name, const float *v3);   ///< GL_FLOAT_VEC3
        void SetVec2 (Program &prog, const char *name, const float *v2);   ///< GL_FLOAT_VEC2
        void SetFloat(Program &prog, const char *name, float v);           ///< GL_FLOAT
        void SetInt  (Program &prog, const char *name, int v);             ///< GL_INT / GL_SAMPLER_*

        /// @brief 캐시된 location 반환. 미존재면 -1 (+ 첫 호출 시 diagnostics 가 warn).
        GLint Get(Program &prog, const char *name);
    }
}

#endif // __SJH_PROGRAM_UNIFORMS_H__
