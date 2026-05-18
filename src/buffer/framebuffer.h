#ifndef __SJH_FRAMEBUFFER_H__
#define __SJH_FRAMEBUFFER_H__

#include "resource_registry/texture.h"

/*********************************************************************************
*
* 렌더링이 될 색상 버퍼에 텍스처를 설정할 수 있음
* 나머지 깊이 / 스텐실 버퍼는 렌더버퍼를 사용
*
*********************************************************************************/

namespace SJH
{
    CLASS_PTR(Framebuffer);
    class Framebuffer
    {
    public:
        static FramebufferUPtr Create(const TexturePtr colorAttachment);
        static void BindToDefault();
        ~Framebuffer();

        const uint32_t Get() const { return mFramebuffer; }
        void Bind() const;
        const TexturePtr GetColorAttachment() const { return mColorAttachment; }

    private:
        Framebuffer() {}
        bool InitWithColorAttachment(const TexturePtr colorAttachment);

        uint32_t mFramebuffer{0};
        uint32_t mDepthStencilBuffer{0};
        TexturePtr mColorAttachment;
    };
}
#endif // __SJH_FRAMEBUFFER_H__
