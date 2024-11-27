#ifndef _BUFFER_H__
#define _BUFFER_H__

#pragma once
#include "common.h"

CLASS_PTR(Buffer)
class Buffer {
public :
    static BufferUPtr CreateWithData(
        uint32_t bufferType, uint32_t usage,
        const void* data, size_t dataSize
    );
    ~Buffer();
    uint32_t Get() const {return mBuffer;}
    void Bind() const;
private :
    enum { DEFAULT_VBO_COUNT = 1 };
    Buffer() {}
    bool Init(
        uint32_t bufferType, uint32_t usage,
        const void* data, size_t dataSize
    );
    uint32_t mBuffer{0};
    uint32_t mBufferType {0};
    uint32_t mUsage {0};        // 이 버퍼가 인덱스인지, 버텍스 버퍼인지 
};

#endif//_BUFFER_H__