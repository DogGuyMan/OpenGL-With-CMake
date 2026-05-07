/**
 * @file program_uniforms.h
 * @brief Program 의 uniform 캐시/setter — 자유 함수 family.
 *
 * @details
 *  ### 디자인 — instance-method 가 아닌 free function 형태
 *  - 모든 함수가 첫 인자로 @c Program& 를 받음 — 상태(캐시)는 Program 의 private 멤버에 존재.
 *  - 함수들은 @c Program 의 *friend* — private 멤버 (@c mProgramAddr, @c mUniformCache) 직접 접근.
 *
 *  ### 책임
 *  - **location 캐시 빌드** — @c BuildCache 가 link 직후 한 번 (eager).
 *    배열 원소 (@c "arr[3]") 같은 비-canonical 이름은 lookup 시점에 lazy 보강.
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
    class Program;   // forward — 사용처는 program.h 가 본 헤더를 include 한 후 정의.

    namespace Uniforms
    {
        /// @brief 캐시 한 칸 — name → (location, type).
        /// @details Program 의 mUniformCache 가 이 타입의 unordered_map. Uniforms 의 자료구조이므로
        ///          Program 안이 아닌 본 namespace 에 정의 — 자유 함수들이 type 직접 사용.
        struct UniformEntry
        {
            GLint  location{-1};   ///< -1 이면 미존재
            GLenum type{0};        ///< 0 이면 active 정보 없음 (lazy 보강 케이스)
        };

        /// @brief Program link 직후 호출 — 모든 active uniform 의 (name, location, type) 캐싱.
        /// @details @c Program::Create 가 자동으로 호출하므로 호출자는 직접 부를 일이 보통 없음.
        ///          link 후 셰이더 source 가 변경되어 다시 link 했다면 재호출 필요.
        void BuildCache(Program &prog);

        // --- setter family — 캐시 히트 시 lookup 비용 0. 미존재 이름은 첫 호출 1회 warn ---
        void SetMat4 (Program &prog, const char *name, const float *mat4); ///< GL_FLOAT_MAT4
        void SetVec4 (Program &prog, const char *name, const float *v4);   ///< GL_FLOAT_VEC4
        void SetVec3 (Program &prog, const char *name, const float *v3);   ///< GL_FLOAT_VEC3
        void SetVec2 (Program &prog, const char *name, const float *v2);   ///< GL_FLOAT_VEC2
        void SetFloat(Program &prog, const char *name, float v);           ///< GL_FLOAT
        void SetInt  (Program &prog, const char *name, int v);             ///< GL_INT / GL_SAMPLER_*

        /// @brief 캐시된 location 반환. 미존재면 -1 (+ 첫 호출 시 diagnostics 가 warn).
        /// @details 배열 원소 같이 eager 캐시에 없는 이름도 lazy lookup 으로 보강.
        GLint Get(Program &prog, const char *name);
    }
}

#endif // __SJH_PROGRAM_UNIFORMS_H__
