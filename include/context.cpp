#include "context.h"

ContextUPtr Context::Create() {
    ContextUPtr context = ContextUPtr(new Context());
    if(!context->Init()) 
        return nullptr;
    return std::move(context);
}

bool Context::Init() {
    float vertices[] = {
         0.5f,  0.5f, 0.0f,  // 우측 상단
         0.5f, -0.5f, 0.0f,  // 우측 하단
        -0.5f, -0.5f, 0.0f,  // 좌측 하단
        -0.5f,  0.5f, 0.0f   // 좌측 상단
    };

    uint32_t indexes[] = {
        0, 1, 3,
        1, 2, 3
    };

    // 1. 지금부터 사용할 VAO 설정
    mVertexLayout = VertexLayout::Create();

    // 2. OpenGL이 사용하기 위해 vertex 리스트를 mVertexBufferObject란 버퍼 개체에 복사
    mVertexBufferObject = Buffer::CreateWithData(GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertices, sizeof(vertices));

    // 3. OpenGL이 사용하기위해 인덱스 리스트를 element 버퍼에 복사
    mIndexBufferObject = Buffer::CreateWithData(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, indexes, sizeof(indexes));

    // 4. Vertex Attribute(정점 속성) 포인터를 세팅
    mVertexLayout->SetAttrib(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);


    ShaderPtr vertShader = Shader::CreateFromFile("./shader/simple.vs", GL_VERTEX_SHADER);
    ShaderPtr fragShader = Shader::CreateFromFile("./shader/simple.fs", GL_FRAGMENT_SHADER);
    if(!vertShader || !fragShader)
        return false;

    SPDLOG_INFO("vertex shader id : {}", vertShader->Get());
    SPDLOG_INFO("fragment shader id : {}", fragShader->Get());

    mProgram = Program::Create({fragShader, vertShader});
    if(!mProgram) 
        return false;
    SPDLOG_INFO("program id : {}", mProgram->Get());
    
    auto location = glGetUniformLocation(mProgram->Get(), "color");
    mProgram->Use();
    glUniform4f(location, 1.0f, 1.0f, 0.0f, 1.0f); // shader의 uniform color 변수에 들어갈 rgba를 float4로 전달

    glClearColor(0.0, 0.1f, 0.2f, 0.0f);
    
    // OpenGL이 primitive를 어떻게 그릴 것인지 설정할 수 있습니다. 
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    return true;
}

void Context::Render() {
    glClear(GL_COLOR_BUFFER_BIT); 

    // 쉐이터 프로그램 테스트
    mProgram->Use();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // uint32_t indexes[]의 자료형을 맞춰줘야 함
}