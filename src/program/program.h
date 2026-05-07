#ifndef __SJH_PROGRAM_H__
#define __SJH_PROGRAM_H__

#include "common/common.h"
#include "shader/shader.h"
#include "program/program_uniforms.h"   // 친구 선언을 위해 Uniforms 자유 함수의 시그니처 필요
#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace SJH
{
    CLASS_PTR(Program)

    /**
     * @brief 컴파일된 셰이더들을 attach + link 한 OpenGL 프로그램 객체의 RAII 래퍼.
     * @details 팩토리 함수 @ref Create 으로만 인스턴스 생성 가능.
     *          외부 노출 인스턴스는 항상 링크까지 완료된 유효한 GL 핸들을 보유한다.
     *          소멸자에서 @c glDeleteProgram 자동 호출.
     *
     *  ### Uniform 캐시 — Uniforms 자유 함수 family 와 협업
     *  - @c mUniformCache 는 *Uniforms 자유 함수* 들이 친구 권한으로 접근하는 private 캐시.
     *  - @c Create 가 link 성공 후 @c Uniforms::BuildCache(*this) 를 호출해 eager build.
     *  - 호출 패턴: @c Uniforms::SetMat4(*prog, "name", data) — 첫 인자가 Program 자신.
     */
    class Program
    {
    public:
        /**
         * @brief 셰이더 벡터를 받아 프로그램 생성 + attach + link 일괄 수행.
         * @param shaders 링크할 셰이더 (vertex / fragment 등 — 보통 2~3개). @c ShaderPtr (shared) 사용.
         * @return 성공 시 @c ProgramUPtr (uniform 캐시 빌드 완료 상태), 링크 실패 시 @c nullptr.
         * @note 링크 에러 로그는 @c diagnostics::GLObjectLog::CheckProgramLink 가 출력.
         * @see Shader::CreateFromFile
         */
        static ProgramUPtr Create(const std::vector<ShaderPtr> &shaders);

        /// @brief @c glDeleteProgram 호출 (핸들이 0 이 아닐 때만).
        ~Program();

        /// @brief 내부 GL 프로그램 핸들 반환 — @c glUseProgram 호출 시 사용.
        GLuint GetProgramAddr() const { return mProgramAddr; }
        void Use() const;

        // === Uniforms 자유 함수의 친구 선언 — private 캐시 직접 접근 권한 부여 ===
        // 정책: Uniforms 함수만 mUniformCache 에 read/write 가능. 외부 코드는 자유 함수 경유.
        friend void  Uniforms::BuildCache(Program &);
        friend void  Uniforms::SetMat4 (Program &, const char *, const float *);
        friend void  Uniforms::SetVec4 (Program &, const char *, const float *);
        friend void  Uniforms::SetVec3 (Program &, const char *, const float *);
        friend void  Uniforms::SetVec2 (Program &, const char *, const float *);
        friend void  Uniforms::SetFloat(Program &, const char *, float);
        friend void  Uniforms::SetInt  (Program &, const char *, int);
        friend GLint Uniforms::Get     (Program &, const char *);

    private:
        Program() = default;
        bool TryLink(const std::vector<ShaderPtr> &shaders);

        GLuint mProgramAddr{0};
        std::unordered_map<std::string, Uniforms::UniformEntry> mUniformCache;
    };
}

#endif // __SJH_PROGRAM_H__
