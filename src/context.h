#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "common.h"
#include "shader.h"
#include "program.h"
#include "buffer.h"
#include "vertex_layout.h"

CLASS_PTR(Context)
class Context {
public :
    static ContextUPtr Create();
    void Render();
private :
    Context() {}
    bool Init();
    ProgramUPtr mProgram;

    VertexLayoutUPtr mVertexLayout;
    BufferUPtr mVertexBufferObject;
    BufferUPtr mIndexBufferObject;
};

#endif //__CONTEXT_H__
