#ifndef __SJH_CONTEXT_H__
#define __SJH_CONTEXT_H__

#include "common/common.h"
#include "program/program.h"
#include "shader/shader.h"

//컨텍스트 클래스 디자인의 철학
/*
* Context가 담당하는것
    * Scene Specific (매우 매우 고변동) 캡슐화
    * VBO/EBO 생성 (Buffer)               // GL_ARRAY_BUFFER / GL_ELEMENT_ARRAY_BUFFER
    * VAO + Vertex Attribute (VertexLayout) // glGenVertexArrays + glVertexAttribPointer
    * 쉐이더/프로그램코드 (Shader/Program)
    * 텍스쳐 로드 + 바인딩 + uniform 설정 (Image / Texture)
    * 매 프레임 draw call 시퀀스 (Render)
    * 위 객체들 자동 파괴 (UPtr 소멸)

* Context가 담당하지 않는것 (앱마다 거~~~의 동일한거)
    * OpenGL 보일러 플레이트 (프로그램마다 필요하지만 거의 동일하게 반복되는 저변동 코드)
    * app/main.cpp의 Life Cycle 영역
    * GLFW init/terminate, OpenGL 컨텍스트 생성, glad 함수 로딩
    * 키/마우스 입력 콜백, 프레임버퍼 리사이즈
    * glfwSwapBuffers / glfwPollEvents 메인 루프
*/
namespace SJH
{
    CLASS_PTR(Context)
    class Context
    {
    public:
        static ContextUPtr Create();
        void Render();

    private:
        Context() = default;
        bool Init();
        ProgramUPtr mProgram;
    };

}

#endif // __CONTEXT_H_
