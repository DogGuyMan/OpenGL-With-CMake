#include "context.h"

ContextUPtr Context::Create() {
    ContextUPtr context = ContextUPtr(new Context());
    if(!context->Init()) 
        return nullptr;
    return std::move(context);
}

bool Context::Init() {
    float vertices[] = {
        -0.5f,  -0.5f,  0.0f,
        0.5f,   -0.5f,  0.0f,
        0.0f,   0.5f,   0.0f,
    };

    glGenVertexArrays(1, &mVertexArrayObject);
    glBindVertexArray(mVertexArrayObject); // 지금부터 사용할 VAO 설정

    glGenBuffers(1, &mVertexBuffer); // 각 정점의 위치값, 색상값을 이런것들이 들어가는데 사용하는 용도.
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer); // mVertexBuffer Object
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*9, vertices, GL_STATIC_DRAW); // 실제로 GPU에 데이터를 복사하는 과정이다.

    // 0 번 attribute를 쓸 것이고 그 attribute는 아래와 같이 생겼다.
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0); 
    // floating point 값이 3개 씩이고, 노멀라이즈, 스트라이드 : sizeof(float) * 3 만큼 건너 뛰고, offset은 0부터 이것이 바로 vertices이 어떻게 구성되어있는지 값을 나타낸다.
    // 그리고 이 "0" 번 attribute는 뭘 가르키냐면 VAO attribute 0번을 의미함
    // simple.vs 버텍스 쉐이더에 layout (location = 0) in vec3 aPos;의 aPos로 들어온다.

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
    
    glClearColor(0.0, 0.1f, 0.2f, 0.0f);

    return true;
}

void Context::Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // 쉐이터 프로그램 테스트
    glUseProgram(mProgram->Get());
    glDrawArrays(GL_TRIANGLES, 0, 4);
}