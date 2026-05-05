#ifndef __SJH_CONTEXT_H__
#define __SJH_CONTEXT_H__

#include "common/common.h"
#include "program/program.h"
#include "shader/shader.h"

namespace SJH
{
    CLASS_PTR(Context)

    /**
     * @brief 한 씬(scene)의 OpenGL 자원과 매 프레임 draw call 시퀀스를 캡슐화하는 클래스.
     *
     * @par 책임 분담 — Context 가 담당하는 것 (Scene-specific, 고변동)
     *  - VBO/EBO 생성 (Buffer)              — @c GL_ARRAY_BUFFER / @c GL_ELEMENT_ARRAY_BUFFER
     *  - VAO + Vertex Attribute (VertexLayout) — @c glGenVertexArrays + @c glVertexAttribPointer
     *  - 셰이더/프로그램 (@ref Shader / @ref Program)
     *  - 텍스처 로드 + 바인딩 + uniform 설정
     *  - 매 프레임 draw call 시퀀스 (@ref Render)
     *  - 위 객체들의 자동 파괴 (@c UPtr 소멸 시점)
     *
     * @par Context 가 담당하지 *않는* 것 — 앱 전반(저변동, 보일러플레이트)
     *  - GLFW @c init/terminate, OpenGL 컨텍스트 생성, glad 함수 로딩
     *  - 키/마우스 입력 콜백, 프레임버퍼 리사이즈
     *  - @c glfwSwapBuffers / @c glfwPollEvents 메인 루프
     *  - 위 항목은 @c app/main.cpp 의 라이프 사이클 영역.
     */
    class Context
    {
    public:
        /**
         * @brief Context 인스턴스 생성 + 내부 GL 자원 초기화 (셰이더 컴파일/링크/VAO 생성).
         * @return 초기화 성공 시 @c ContextUPtr, 셰이더/프로그램 생성 실패 시 @c nullptr.
         * @pre 호출 전 OpenGL 컨텍스트가 활성화되어 있고 glad 가 로드된 상태여야 함.
         */
        static ContextUPtr Create();

        /// @brief 매 프레임 호출 — 프레임버퍼 클리어 + draw call.
        void Render();

    private:
        Context() = default;
        bool Init();
        ProgramUPtr mProgram;
    };
}

#endif // __SJH_CONTEXT_H__
