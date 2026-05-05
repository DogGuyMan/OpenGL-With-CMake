#ifndef __SJH_PROGRAM_H__
#define __SJH_PROGRAM_H__

#include "common/common.h"
#include "shader/shader.h"
#include <vector>
#include <glad/glad.h>

namespace SJH
{
    CLASS_PTR(Program)
    class Program
    {
    public:
        static ProgramUPtr Create(const std::vector<ShaderPtr> &shaders);

        ~Program();
        GLuint GetProgramAddr() const { return mProgramAddr; }

    private:
        Program() = default;
        // Shader Attatch 작업 수행
        bool TryLink(const std::vector<ShaderPtr> &shaders);
        GLuint mProgramAddr{0};
    };
}

#endif // __SJH_PROGRAM_H__
