#include "image.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

ImageUPtr Image::Load(const std::string& filepath) {
    ImageUPtr image = ImageUPtr(new Image());
    if (!image->LoadWithStb(filepath))
        return nullptr;
    return std::move(image);
}

ImageUPtr Image::Create(int width, int height, int channelCount) {
    ImageUPtr image = ImageUPtr(new Image());
    if (!image->Allocate(width, height, channelCount))
        return nullptr;
    return std::move(image);
}

Image::~Image() {
    if (mData) {
        stbi_image_free(mData);
    }
}

void Image::SetCheckImage(int gridX, int gridY) {
    for (int j = 0; j < mHeight; j++) {
        for (int i = 0; i < mWidth; i++) {
            int pos = (j * mWidth + i) * mChannelCount;
            bool even = ((i / gridX) + (j / gridY)) % 2 == 0;
            uint8_t value = even ? 255 : 0;
            for (int k = 0; k < mChannelCount; k++)
                mData[pos + k] = value;
            if (mChannelCount > 3)
                mData[3] = 255;
        }
    }
}

bool Image::LoadWithStb(const std::string& filepath) {
    stbi_set_flip_vertically_on_load(true);
    mData = stbi_load(filepath.c_str(), &mWidth, &mHeight, &mChannelCount, 0);
    if (!mData) {
        SPDLOG_ERROR("failed to load image: {}", filepath);
        return false;
    }
    return true;
}

bool Image::Allocate(int width, int height, int channelCount) {
    mWidth = width;
    mHeight = height;
    mChannelCount = channelCount;
    mData = (uint8_t*)malloc(mWidth * mHeight * mChannelCount);
    return mData ? true : false;
}
