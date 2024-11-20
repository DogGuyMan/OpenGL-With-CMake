#ifndef __PROGRAM_H__
#define __PROGRAM_H__

#include "common.h"
#include "shader.h"

CLASS_PTR(Program) 
class Program {
public :
    // 개체를 생성하는 생성자 역할을 하고, 
    // 반환형으로 Unique Pointer 형태 리턴하는 팩토리
    // vector 타입으로 여러개의 쉐이더 포인터를 받는다.
    static ProgramUPtr Create(const std::vector<ShaderPtr> & shaders);
    ~Program();
    uint32_t Get() const {return mProgram;}
private:
    // Program 개채 생성을 Create라는 함수로 책임 전가 따라서 
    // 내부가 아닌 외부에서 기본 생성자로 만든다는 것은 에러임
    // 단, 내부에서는 생성자 호출을 할 수 있다는것
    Program() {}
    // 두개의 쉐이더 타입을 받아서 링크하는것
    bool Link(const std::vector<ShaderPtr>& shaders);
    uint32_t mProgram {0};
};

#endif // __PROGRAM_H__