#include "context.h"
#include "buffer/buffer.h"
#include "diagnostics/gl_log.h"
#include "layout/vertex_layout.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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
    float vertices[] = {
    -0.5f, -0.5f, -0.5f, 1.0, 1.0, 1.0, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0, 1.0, 1.0, 1.0f, 0.0f,
    0.5f,  0.5f, -0.5f, 1.0, 1.0, 1.0, 1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f, 1.0, 1.0, 1.0, 0.0f, 1.0f,

    -0.5f, -0.5f,  0.5f, 1.0, 1.0, 1.0, 0.0f, 0.0f,
    0.5f, -0.5f,  0.5f, 1.0, 1.0, 1.0, 1.0f, 0.0f,
    0.5f,  0.5f,  0.5f, 1.0, 1.0, 1.0, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f, 1.0, 1.0, 1.0, 0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f, 1.0, 1.0, 1.0, 1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, 1.0, 1.0, 1.0, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 1.0, 1.0, 1.0, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, 1.0, 1.0, 1.0, 0.0f, 0.0f,

    0.5f,  0.5f,  0.5f, 1.0, 1.0, 1.0, 1.0f, 0.0f,
    0.5f,  0.5f, -0.5f, 1.0, 1.0, 1.0, 1.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 1.0, 1.0, 1.0, 0.0f, 1.0f,
    0.5f, -0.5f,  0.5f, 1.0, 1.0, 1.0, 0.0f, 0.0f,

    -0.5f, -0.5f, -0.5f, 1.0, 1.0, 1.0, 0.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 1.0, 1.0, 1.0, 1.0f, 1.0f,
    0.5f, -0.5f,  0.5f, 1.0, 1.0, 1.0, 1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f, 1.0, 1.0, 1.0, 0.0f, 0.0f,

    -0.5f,  0.5f, -0.5f, 1.0, 1.0, 1.0, 0.0f, 1.0f,
    0.5f,  0.5f, -0.5f, 1.0, 1.0, 1.0, 1.0f, 1.0f,
    0.5f,  0.5f,  0.5f, 1.0, 1.0, 1.0, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, 1.0, 1.0, 1.0, 0.0f, 0.0f,
    };

    // ! ⭐️ 제발 인덱스 버퍼는 GLuint로 두자 ⭐️
    uint32_t indices[] = {
    0,  2,  1,  2,  0,  3,
    4,  5,  6,  6,  7,  4,
    8,  9, 10, 10, 11,  8,
    12, 14, 13, 14, 12, 15,
    16, 17, 18, 18, 19, 16,
    20, 22, 21, 22, 20, 23,
    };
    ContextUPtr Context::Create()
    {
        auto context = ContextUPtr(new Context());
        if (!context->Init())
            return nullptr;
        return std::move(context);
    }

    void Context::Render()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        static float time = 0.0f;
        float t = sinf(time) * 0.5f + 0.5f;

        auto trans = glm::translate(glm::mat4(1.0f), glm::vec3(cos(time), sin(time), 0.0f) * 0.5f);
        // 단위행렬 기준 z축으로 90도만큼 회전하는 행렬
        auto rot = glm::rotate(glm::mat4(1.0f),
                               glm::radians(90.0f) * time, glm::vec3(1.0f, 0.0f, 0.0f));
        // 단위행렬 기준 모든 축에 대해 3배율 확대하는 행렬
        auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));

        auto transformMat = trans * rot * scale;

        // 카메라: z=3 위치에서 원점을 바라봄
        auto viewMat = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f), // eye
            glm::vec3(0.0f, 0.0f, 0.0f), // center
            glm::vec3(0.0f, 1.0f, 0.0f)  // up
        );
        // 원근 투영: 45도 FOV, 4:3 종횡비, near=0.1 far=100
        auto projMat = glm::perspective(
            glm::radians(45.0f),
            4.0f / 3.0f,
            0.1f, 100.0f);

        // 1. 바인드 하는곳
        {
            mVertexArrayObject->Bind();
            // auto texturePtr = mRM->LoadTextureWithName("container");
            {
                // glActiveTexture(textureSlot) 함수로 현재 다루고자 하는 텍스처 슬롯을 선택
                glActiveTexture(GL_TEXTURE0);
                auto texturePtr = mRM->LoadTextureWithName("checkerboard");
                // glBindTexture(textureType, textureId) 함수로 현재 설정중인 텍스처 슬롯에 우리의 텍스처 오브젝트를 바인딩
                texturePtr->Bind(); // -> glBindTexture(GL_TEXTURE_2D, mTextureID);
            }
            {
                // glActiveTexture(textureSlot) 함수로 현재 다루고자 하는 텍스처 슬롯을 선택
                glActiveTexture(GL_TEXTURE1);
                // glBindTexture(textureType, textureId) 함수로 현재 설정중인 텍스처 슬롯에 우리의 텍스처 오브젝트를 바인딩
                auto texturePtr = mRM->LoadTextureWithName("awesomeface");
                texturePtr->Bind(); // -> glBindTexture(GL_TEXTURE_2D, mTextureID);
            }
        }

        // 2. Use Program
        mProgram->Use();

        // glUniform* 은 현재 use 중인 프로그램에만 적용 — Use() 선행 필수.

        // glPointSize(10.0f);
        // glDrawArrays(GL_POINTS, 0, 1);

        // VBO 만으로 그렸을때
        // glDrawArrays(GL_TRIANGLES, 0, 6);

        // 3. Uniform 전달
        auto transformLoc = glGetUniformLocation(mProgram->GetProgramAddr(), "modelMat");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transformMat));
        auto viewLoc = glGetUniformLocation(mProgram->GetProgramAddr(), "viewMat");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMat));
        auto projLoc = glGetUniformLocation(mProgram->GetProgramAddr(), "projMat");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projMat));

        glm::vec4 baseColor(t * t, 2.0f * t * (1.0f - t), (1.0f - t) * (1.0f - t), 1.0f);

        auto baseColorLoc = glGetUniformLocation(mProgram->GetProgramAddr(), "baseColor");
        glUniform4fv(baseColorLoc, 1, glm::value_ptr(baseColor));

        for (int i = 0; i < 2; i++)
        {
            std::string texuniform = "tex" + std::to_string(i);
            // glGetUniformLocation() 함수로 shader 내의 sampler2D uniform 핸들을 얻어옴
            auto loc = glGetUniformLocation(mProgram->GetProgramAddr(), texuniform.c_str());
            // glUniform1i() 함수로 sampler2D uniform에 텍스처 슬롯 인덱스를 입력
            glUniform1i(loc, i);
        }

        // VBO + EBO 협력으로 그렸을때.
        // {
        //     glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // }
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        time += 0.016f;
    }

    bool Context::Init()
    {
        // ShaderUPtr -> ShaderPtr 암묵 변환 — Program::Create 가 shared 입력을 요구
        ShaderPtr vertexShader = Shader::CreateFromFile("./resources/shader/simple.vs", GL_VERTEX_SHADER);
        ShaderPtr fragmentShader = Shader::CreateFromFile("./resources/shader/simple.fs", GL_FRAGMENT_SHADER);
        if (!vertexShader || !fragmentShader)
            return false;
        spdlog::info("vertex shader id: {}", vertexShader->GetShaderAddr());
        spdlog::info("fragment shader id: {}", fragmentShader->GetShaderAddr());

        mProgram = Program::Create({vertexShader, fragmentShader});
        if (!mProgram)
            return false;
        spdlog::info("program id: {}", mProgram->GetProgramAddr());
        glClearColor(0.0, 0.1f, 0.2f, 0.0f);

        mRM = ResourceManagement::CreateRM();

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

        // {
        //     const GLint buffer_size = vertices.size() * sizeof(GLfloat);
        //     mVertexBufferObject = Buffer::CreateWithData(GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertices.data(), buffer_size);

        //     const GLint element_size = indices.size() * sizeof(GLuint);
        //     mElementBufferObject = Buffer::CreateWithData(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, indices.data(), element_size);

        //     // === 셰이더 쪽 layout(location = 0) 인곳에 넣겠다. ===
        //     void *positionOffset = 0;
        //     // GLuint positionStride = sizeof(glm::vec3);

        //     if (!mVertexArrayObject->TrySetAttrib(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, 0))
        //         return false;
        //     if (!mVertexArrayObject->TrySetAttrib(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, sizeof(GLfloat) * 3))
        //         return false;
        //     if (!mVertexArrayObject->TrySetAttrib(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, sizeof(GLfloat) * 6))
        //         return false;
        // }

        mVertexBufferObject = Buffer::CreateWithData(
            GL_ARRAY_BUFFER, GL_STATIC_DRAW,
            vertices, sizeof(float) * 196);

        mElementBufferObject = Buffer::CreateWithData(
            GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW,
            indices, sizeof(uint32_t) * 36);

        mVertexArrayObject->TrySetAttrib(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, 0);
        mVertexArrayObject->TrySetAttrib(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, sizeof(float) * 3);
        mVertexArrayObject->TrySetAttrib(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, sizeof(float) * 6);

        auto imagePtr1 = mRM->LoadImage("container", "./resources/texture/container.jpg");
        if (imagePtr1 == nullptr)
            return false;
        spdlog::info("ImageName \'{}\': {}x{}, {} channels",
                     imagePtr1->GetImageName(),
                     imagePtr1->GetWidth(),
                     imagePtr1->GetHeight(),
                     imagePtr1->GetChannelCount());

        auto imagePtr2 = mRM->LoadImage("awesomeface", "./resources/texture/awesomeface.png");
        if (imagePtr2 == nullptr)
            return false;
        spdlog::info("ImageName \'{}\': {}x{}, {} channels",
                     imagePtr2->GetImageName(),
                     imagePtr2->GetWidth(),
                     imagePtr2->GetHeight(),
                     imagePtr2->GetChannelCount());

        auto checkerImgPtr = Image::Create("checkerboard", 512, 512);
        checkerImgPtr->SetCheckImage(16, 16);

        mRM->LoadTextureFromImage(imagePtr1);
        mRM->LoadTextureFromImage(imagePtr2);
        mRM->LoadTextureFromImage(checkerImgPtr.get());

        auto texturePtr = mRM->LoadTextureWithName("awesomeface");

        glActiveTexture(GL_TEXTURE0);
        texturePtr->Bind();
        mProgram->Use();
        glUniform1i(glGetUniformLocation(mProgram->GetProgramAddr(), "tex"), 0);
        return true;
    }
}
