#include "program.h"

ProgramUPtr Program::Create(const std::vector<ShaderPtr> &shaders)
{
    ProgramUPtr program = ProgramUPtr(new Program());
    if (!program->Link(shaders))
        return nullptr;
    return std::move(program);
}

Program::~Program() {
    if(mProgram) {
        glDeleteProgram(mProgram);
    }
}

bool Program::Link(const std::vector<ShaderPtr> &shaders)
{
    // glCreateProgram 으로 새로운 OpenGL program object 생성
    mProgram = glCreateProgram();
    for (const auto &shader : shaders) {
        // glAttachShader program에 shader 붙이기 shader object id값을 하나씩 하나씩 세팅을 해준다.
        glAttachShader(mProgram, shader->Get()); 
    }

    // glLinkProgram으로 프로그램 링크
    glLinkProgram(mProgram); 

    // glGetProgramiv으로 프로그램 링크 상태 확인
    // 실패했다면 에러 Log 띄우기
    int success = 0;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &success); 
    if (!success) 
    {
        char infoLog[1024];
        glGetProgramInfoLog(mProgram, 1024, nullptr, infoLog);
        SPDLOG_ERROR("failed to link program: {}", infoLog);
        return false;
    }

    return true; 
}

void Program::Use() const {
    glUseProgram(mProgram);
}