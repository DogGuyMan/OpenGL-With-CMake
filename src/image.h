#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "common.h"

CLASS_PTR(Image)
class Image {
public:
    static ImageUPtr Load(const std::string& filepath);
    static ImageUPtr Create(int width, int height, int channelCount = 4);
    ~Image();

    const uint8_t* GetData() const { return mData; }
    int GetWidth() const { return mWidth; }
    int GetHeight() const { return mHeight; }
    int GetChannelCount() const { return mChannelCount; }

    void SetCheckImage(int gridX, int gridY);

private:
    Image() {};
    bool LoadWithStb(const std::string& filepath);
    bool Allocate(int width, int height, int channelCount);
    int mWidth { 0 };
    int mHeight { 0 };
    int mChannelCount { 0 };
    uint8_t* mData { nullptr };
};

#endif // __IMAGE_H__
