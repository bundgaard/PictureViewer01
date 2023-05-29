#include "AnimatedImage.h"
#include "GraphicFactory.h"
#include "GraphicsManager.h"

#include <stdexcept>

AnimatedImage::AnimatedImage(GraphicFactory& graphicsFactory)
	: mGraphicsFactory(graphicsFactory)
	, mFrameCount(0)
{	
}

AnimatedImage::~AnimatedImage()
{	
	if (!mFrames.empty())
	{
		for (auto*& frame : mFrames)
		{
			frame->Release();
		}
	}
}

void AnimatedImage::Load(std::wstring const& filepath)
{
	HRESULT hr = S_OK;

	IWICBitmapDecoder* decoder = nullptr;

	if (SUCCEEDED(hr))
	{
		hr = mGraphicsFactory.GetWICFactory()->CreateDecoderFromFilename(
			filepath.c_str(),
			nullptr,
			GENERIC_READ,
			WICDecodeMetadataCacheOnDemand,
			&decoder);
	}

	if (FAILED(hr))
	{
		throw std::runtime_error("failed to load file.");
	}

	if (SUCCEEDED(hr))
	{
		hr = decoder->GetFrameCount(&mFrameCount);
	}

	if (SUCCEEDED(hr))
	{

		IWICBitmapFrameDecode* frame = nullptr;
		for (int i = 0; i < mFrameCount; i++)
		{
			hr = decoder->GetFrame(i, &frame);
			if (SUCCEEDED(hr))
			{
				mFrames.emplace_back(std::move(frame));
			}
		}

	}

	IWICFormatConverter* converter = nullptr;
	if (SUCCEEDED(hr))
	{
		hr = mGraphicsFactory.GetWICFactory()->CreateFormatConverter(&converter);
	}

	converter->Initialize(
		mFrames[0],
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0,
		WICBitmapPaletteTypeCustom);
}

void AnimatedImage::Render()
{
	// here we should draw a picture and then draw it to our HwndRenderTarget
}

void AnimatedImage::Update()
{
	// This should be called in a timer from the Viewer class
}
