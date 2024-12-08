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
    if(!image->Allocate(width, height, channelCount))
        return nullptr;
    return std::move(image);
}

Image::~Image() {
    if (mData) {
        stbi_image_free(mData);
    }
}

void Image::SetCheckImage(int gridX, int gridY) {

        for(int i = 0; i < mWidth; i++) {
            for(int j = 0; j < mHeight; j++) {
                int pos = (j * mWidth + i) * mChannelCount;
                uint8_t color = (((i / gridX) % 2) ^ ((j / gridY) % 2)) * 255;
                for(int k = 0; k < mChannelCount; k++)
                    mData[pos + k] = color;
                if(mChannelCount > 4)
                    mData[3] = 255;
            }
        }
}

bool Image::LoadWithStb(const std::string& filepath) {
    // 이 코드는 좌 상단으로 이미지를 원점으로 설정하는 코드다.
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
    /*
    텍스쳐가 차지하는 메모리공간을 확보하는 것이다.
    자, 이말을 보아하면 가로 x 세로 개수만큼 byte를 할당하지?
    끝이냐? ㄴㄴ R, G, B, A 각각 또 할당해줘야 한다.
    만약 단일 채널만 사용한다면 끝이지만, 빨,초,파,투명 이것들이 각각 픽셀마다 몇의 값을 가지고 있는지 저장한다면?
    그 채널 개수만큼 Byte를 또 할당해야 한다.

    그리고 나운 malloc void * 타입으로 나온것을 uint8_t포인터로 캐스팅
    */
    mData = (uint8_t*)malloc(mWidth * mHeight * mChannelCount);
    return mData ? true : false;
}
