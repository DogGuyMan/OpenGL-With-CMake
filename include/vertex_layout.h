#ifndef __VERTEX_LAYOUT_H__
#define __VERTEX_LAYOUT_H__

#pragma once
#include "common.h"

CLASS_PTR(VertexLayout)
class VertexLayout {
public :
    static VertexLayoutUPtr Create();
    ~VertexLayout();

    uint32_t Get() const {return mVertexArrayObject;}
    void Bind() const;
    void SetAttrib(
        uint32_t attribIndex, int count,
        uint32_t type, bool normalized,
        size_t stride, uint64_t offset) const;
    void DisableAttrib(int attribIndex) const;
private :
    enum { DEFAULT_VAO_COUNT = 1 };
    VertexLayout() {}
    void Init();
    uint32_t mVertexArrayObject {0};
};

#endif//__VERTEX_LAYOUT_H__