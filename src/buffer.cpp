
#include "buffer.h"

// void* data에는 vertices[], indexes[] 이런것들이 들어감
BufferUPtr Buffer::CreateWithData(
    uint32_t bufferType, uint32_t usage,
    const void* data, size_t dataSize
) {
    BufferUPtr buffer = BufferUPtr(new Buffer());
    if(!buffer->Init(bufferType, usage, data, dataSize)){
        return nullptr;
    }

    return std::move(buffer);
}

Buffer::~Buffer() {
    if(mBuffer) {
        glDeleteBuffers(DEFAULT_VBO_COUNT, &mBuffer);
    }
}

void Buffer::Bind() const {
    glBindBuffer(mBufferType, mBuffer);
}

// OpenGL이 사용하기 위해 vertex/index 리스트를 버퍼 개체에 복사
// glBufferData는 실제로 GPU에 데이터를 복사하는 과정이다.
bool Buffer::Init(
    uint32_t bufferType, uint32_t usage,
    const void* data, size_t dataSize
) {
    mBufferType = bufferType;
    mUsage = usage;
    // 각 정점의 위치값, 색상값을 이런것들이 들어가는 버퍼 생성
        //n : 구체적인 VBO("buffer object") 개수
        //buffers : VBO배열
    glGenBuffers(DEFAULT_VBO_COUNT, &mBuffer);
    Bind();
    glBufferData(bufferType, dataSize, data, usage);
    return true;
}