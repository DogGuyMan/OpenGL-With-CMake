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

    uint32_t mVertexArrayObject;
    uint32_t mVertexBuffer;
};

#endif //__CONTEXT_H__