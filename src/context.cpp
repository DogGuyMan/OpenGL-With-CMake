#include "context.h"

ContextUPtr Context::Create() {
    ContextUPtr context = ContextUPtr(new Context());
    if(!context->Init())
        return nullptr;
    return std::move(context);
}

bool Context::Init() {
    // 위치 {0.5f, 0.5f, 0.0f} | 컬러 {1.0f, 0.0f, 0.0f}
    float vertices[] = {
        0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right, red
        0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right, green
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left, blue
        -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0, 1.0f, // top left, yellow
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
    // mVertexLayout->SetAttrib(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

    // 오프셋 이동 안해도 되는 position
    mVertexLayout->SetAttrib(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, 0);
    // offset 이동해야하는 vertex color
    mVertexLayout->SetAttrib(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, sizeof(float) * 3);
    // color 다음으로 s, t 딱 2개 더 추가됨 2임
    // 그리고 offset 이동해야하는 vertex s,t 는 position과 color를 건너 뛰어야 하므로 6
    mVertexLayout->SetAttrib(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, sizeof(float) * 6);


    ShaderPtr vertShader = Shader::CreateFromFile("./shader/texture.vs", GL_VERTEX_SHADER);
    ShaderPtr fragShader = Shader::CreateFromFile("./shader/texture.fs", GL_FRAGMENT_SHADER);
    if(!vertShader || !fragShader)
        return false;

    SPDLOG_INFO("vertex shader id : {}", vertShader->Get());
    SPDLOG_INFO("fragment shader id : {}", fragShader->Get());

    mProgram = Program::Create({fragShader, vertShader});
    if(!mProgram)
        return false;
    SPDLOG_INFO("program id : {}", mProgram->Get());

    glClearColor(0.0, 0.1f, 0.2f, 0.0f);

    // OpenGL이 primitive를 어떻게 그릴 것인지 설정할 수 있습니다.
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    //이미지를 CPU 쪽에 로딩한다.
    ImageUPtr image = Image::Load("./image/container.jpg");
    if(!image)
        return false;
    SPDLOG_INFO("image: {}x{}, {} channels", image->GetWidth(), image->GetHeight(), image->GetChannelCount());

    //이미지를 GPU로 복사한다
    mTexture = Texture::CreateFromImage(image.get());
    return true;
}

void Context::Render() {
    glClear(GL_COLOR_BUFFER_BIT);
    mProgram->Use();
    glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
