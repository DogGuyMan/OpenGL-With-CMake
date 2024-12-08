#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "image.h"

CLASS_PTR(Texture)
class Texture {
public :
    static TextureUPtr CreateFromImage(const Image* image); // 이번에는 UPtr이 아니네?
    ~Texture();

    const uint32_t Get() const {return mTexture;}
    void Bind() const;
    void SetFilter(uint32_t minFilter, uint32_t magFilter) const;
    void SetWrap(uint32_t sWarp, uint32_t tWarp) const;
private :
    enum { DEFAULT_GL_SIZE_I = 1 };
    Texture() {}
    void CreateTexture();
    void SetTextureFromImage(const Image* image);
    uint32_t mTexture {0};
};

#endif//__TEXTURE_H__
