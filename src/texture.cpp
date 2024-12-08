#include "texture.h"

TextureUPtr Texture::CreateFromImage(const Image* image) {
    TextureUPtr texture = TextureUPtr(new Texture());
    texture->CreateTexture();
    texture->SetTextureFromImage(image);
    return std::move(texture);
}

Texture::~Texture() {
    if(mTexture) {
        glDeleteTextures(DEFAULT_GL_SIZE_I, &mTexture);
    }
}

void Texture::Bind() const {
    glBindTexture(GL_TEXTURE_2D, mTexture);
}

// 외부에서 초기화 해도 된다
void Texture::SetFilter(uint32_t minFilter, uint32_t magFilter) const {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter); // 축소시 보간할 필터 방법
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter); // 확대시 보간할 필터 방법

}

// 외부에서 초기화 해도 된다
void Texture::SetWrap(uint32_t sWarp, uint32_t tWarp) const {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWarp); // 텍스쳐 U 가 벗어났을떄 image를 어떻게 사용할것인가.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWarp); // 텍스쳐 V 가 벗어났을떄 image를 어떻게 사용할것인가.
}

void Texture::CreateTexture() {
    glGenTextures(DEFAULT_GL_SIZE_I, &mTexture);
    Bind();
    SetFilter(GL_LINEAR, GL_LINEAR);
    SetWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
}

void Texture::SetTextureFromImage(const Image* image) {
    GLenum format = GL_RGBA;
    switch(image->GetChannelCount()) {
        default: break;
        case 1 : {format = GL_RED;  break;}
        case 2 : {format = GL_RG;   break;}
        case 3 : {format = GL_RGB;  break;}
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        image->GetWidth(), image->GetHeight(), 0,
        format, GL_UNSIGNED_BYTE, image->GetData());
}
