#include "PixelImage.h"
#include <memory>

PixelImage::PixelImage(PixelImage&& other) noexcept : mWidth(0), mHeight(0)
{
	*this = std::move(other);
}

/*
ScratchImage& ScratchImage::operator= (ScratchImage&& moveFrom) noexcept
{
    if (this != &moveFrom)
    {
        Release();

        m_nimages = moveFrom.m_nimages;
        m_size = moveFrom.m_size;
        m_metadata = moveFrom.m_metadata;
        m_image = moveFrom.m_image;
        m_memory = moveFrom.m_memory;

        moveFrom.m_nimages = 0;
        moveFrom.m_size = 0;
        moveFrom.m_image = nullptr;
        moveFrom.m_memory = nullptr;
    }
    return *this;
}

*/

PixelImage& PixelImage::operator=(PixelImage&& other) noexcept
{
    if (this != &other)
    {
        mWidth = other.mWidth;
        mHeight = other.mHeight;
    }
    return *this;
}

PixelImage::~PixelImage()
{
}
