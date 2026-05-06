#include "context.h"
#include "buffer/buffer.h"
#include "layout/vertex_layout.h"
#include "diagnostics/gl_log.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

namespace SJH
{
    /*********************************************************************************
     *
     * ! ⭐️ 제발 인덱스 버퍼는 GLuint로 두자 GLfloat으로 잘못 두지 말고 😭 ⭐️
     * ! ⭐️ 제발 정점은 float으로 놓고 ⭐️
     * ! 이런 것때문에 Buffer 같은 강한 타입으로 클래스 디자인을 해야함.
     *
     *********************************************************************************/
    // 엥? 교안 CW 배치네..
    static std::array<glm::vec3, 4> vertices
    {
        glm::vec3(0.5f, 0.5f, 0.0f),   // top right
        glm::vec3(0.5f, -0.5f, 0.0f),  // bottom right
        glm::vec3(-0.5f, -0.5f, 0.0f), // bottom left
        glm::vec3(-0.5f, 0.5f, 0.0f),  // top left
    };

    // ! ⭐️ 제발 인덱스 버퍼는 GLuint로 두자 ⭐️
    static std::array<GLuint, 6> indices = {
        0, 1, 3, 1, 2, 3};

    ContextUPtr Context::Create()
    {
        auto context = ContextUPtr(new Context());
        if (!context->Init())
            return nullptr;
        return std::move(context);
    }

    void Context::Render()
    {
        glClear(GL_COLOR_BUFFER_BIT);

        mProgram->Use();

        // glPointSize(10.0f);
        // glDrawArrays(GL_POINTS, 0, 1);

        // VBO 만으로 그렸을때
        // glDrawArrays(GL_TRIANGLES, 0, 6);

        // VBO + EBO 협력으로 그렸을때.
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    bool Context::Init()
    {
        // ShaderUPtr -> ShaderPtr 암묵 변환 — Program::Create 가 shared 입력을 요구
        SJH::ShaderPtr vertexShader = SJH::Shader::CreateFromFile("./resources/shader/simple.vs", GL_VERTEX_SHADER);
        SJH::ShaderPtr fragmentShader = SJH::Shader::CreateFromFile("./resources/shader/simple.fs", GL_FRAGMENT_SHADER);
        if (!vertexShader || !fragmentShader)
            return false;
        SPDLOG_INFO("vertex shader id: {}", vertexShader->GetShaderAddr());
        SPDLOG_INFO("fragment shader id: {}", fragmentShader->GetShaderAddr());

        mProgram = SJH::Program::Create({vertexShader, fragmentShader});
        if (!mProgram)
            return false;
        SPDLOG_INFO("program id: {}", mProgram->GetProgramAddr());
        glClearColor(0.0, 0.1f, 0.2f, 0.0f);

        /*
        순서
            1. VAO
            2. VBO
            3. Vertex Attribute Setting
        */

        mVertexArrayObject = VertexLayout::Create();

        // === buffer object를 만든다 ===
        // === 지금부터 사용할 buffer object를 지정한다. ===
        // === 사용할 buffer object는 vertex data를 저장할 용도 ===
        // === 변경빈도(STATIC=한번 / DYNAMIC=자주 / STREAM=매프레임) x 용도(DRAW=앱-> GPU / READ=GPU -> 앱 / COPY=GPU <-> GPU) ===
        const GLint buffer_size = vertices.size() * sizeof(glm::vec3);
        mVertexBufferObject = Buffer::CreateWithData(GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertices.data(), buffer_size);

        const GLint element_size = indices.size() * sizeof(GLuint);
        mElementBufferObject = Buffer::CreateWithData(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, indices.data(), element_size);

        // === 셰이더 쪽 layout(location = 0) 인곳에 넣겠다. ===
        void *positionOffset = 0;
        GLuint positionStride = sizeof(glm::vec3);

        if(!mVertexArrayObject->TrySetAttrib(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0))
            return false;

        return true;
    }
}
