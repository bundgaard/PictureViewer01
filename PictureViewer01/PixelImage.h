#pragma once
#define NOMINMAX
#define NOHELP
#define NOMCX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class PixelImage
{
	UINT mWidth;
	UINT mHeight;

public:

	PixelImage(PixelImage&&) noexcept;
	PixelImage& operator=(PixelImage&&)noexcept;
	~PixelImage();

	PixelImage(PixelImage&) = delete;
	PixelImage& operator=(PixelImage&) = delete;
};

