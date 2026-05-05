#ifndef __SJH_PROGRAM_H__
#define __SJH_PROGRAM_H__

#include "common/common.h"
#include "shader/shader.h"
#include <vector>
#include <glad/glad.h>

namespace SJH
{
    CLASS_PTR(Program)

    /**
     * @brief 컴파일된 셰이더들을 attach + link 한 OpenGL 프로그램 객체의 RAII 래퍼.
     * @details 팩토리 함수 @ref Create 으로만 인스턴스 생성 가능.
     *          외부 노출 인스턴스는 항상 링크까지 완료된 유효한 GL 핸들을 보유한다.
     *          소멸자에서 @c glDeleteProgram 자동 호출.
     */
    class Program
    {
    public:
        /**
         * @brief 셰이더 벡터를 받아 프로그램 생성 + attach + link 일괄 수행.
         * @param shaders 링크할 셰이더 (vertex / fragment 등 — 보통 2~3개). @c ShaderPtr (shared) 사용.
         * @return 성공 시 @c ProgramUPtr, 링크 실패 시 @c nullptr.
         * @details 입력이 @c shared_ptr 이므로 호출자가 같은 셰이더를 여러 프로그램에 재사용 가능.
         *          링크 후에는 @c glDetachShader 호출 없이도 셰이더 객체 파괴 시 자동 분리됨.
         * @note 링크 에러 로그는 @c diagnostics::GLObjectLog::CheckProgramLink 가 출력.
         * @see Shader::CreateFromFile
         */
        static ProgramUPtr Create(const std::vector<ShaderPtr> &shaders);

        /// @brief @c glDeleteProgram 호출 (핸들이 0 이 아닐 때만).
        ~Program();

        /// @brief 내부 GL 프로그램 핸들 반환 — @c glUseProgram 호출 시 사용.
        GLuint GetProgramAddr() const { return mProgramAddr; }

    private:
        Program() = default;
        bool TryLink(const std::vector<ShaderPtr> &shaders);
        GLuint mProgramAddr{0};
    };
}

#endif // __SJH_PROGRAM_H__
