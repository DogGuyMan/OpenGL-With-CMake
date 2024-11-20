#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "common.h"
#include "shader.h"
#include "program.h"

CLASS_PTR(Context)
class Context {
public :
    static ContextUPtr Create();
    void Render();
private :  
    Context() {}
    bool Init();
    ProgramUPtr mProgram;
};

#endif //__CONTEXT_H__