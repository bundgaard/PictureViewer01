#include "AnimatedImage.h"
#include "GraphicFactory.h"
#include "GraphicsManager.h"
#include "Log.h"
#include <stdexcept>
#include <atlbase.h>

AnimatedImage::AnimatedImage(GraphicFactory& graphicsFactory)
	: mGraphicsFactory(graphicsFactory)
	, mFrameCount(0)
{


}

AnimatedImage::~AnimatedImage()
{
	LOG(L"AnimatedImage Destructor\n");
	if (!mFrames.empty())
	{
		for (auto*& frame : mFrames)
		{
			frame->Release();
			frame = nullptr;
		}
	}
}

void AnimatedImage::Load(std::wstring const& filepath, ID2D1HwndRenderTarget* renderTarget)
{
	HRESULT hr = S_OK;
	
	CComPtr<IWICBitmapDecoder> decoder;
	CComPtr<IWICFormatConverter> formatConverter;

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
	LOG(L"Framecount %u\n", mFrameCount);

	
	if (SUCCEEDED(hr))
	{
		HRESULT hr = S_OK;


		if (SUCCEEDED(hr))
		{
			for (unsigned i = 0; i < mFrameCount; ++i)
			{


				CComPtr<IWICBitmapFrameDecode> frame;
				CComPtr<ID2D1Bitmap> bitmap;


				if (SUCCEEDED(hr))
				{
					hr = decoder->GetFrame(i, &frame);
				}


				WICPixelFormatGUID format;
				if (SUCCEEDED(hr))
				{
					hr = frame->GetPixelFormat(&format);
				}

				WICColor rgbColors[256] = {};
				UINT actualColors = 0;
				{
					CComPtr<IWICPalette> palette;
					
					hr = mGraphicsFactory.GetWICFactory()->CreatePalette(&palette);
					if (SUCCEEDED(hr))
					{
						hr = decoder->CopyPalette(palette);
					}

					if (SUCCEEDED(hr))
					{
						hr = palette->GetColors(static_cast<UINT>(std::size(rgbColors)), rgbColors, &actualColors);
					}
				}
				OutputDebugStringW(L"Got the palette\n");
				if (SUCCEEDED(hr))
				{
					hr = mGraphicsFactory.GetWICFactory()->CreateFormatConverter(&formatConverter); // TODO: maybe a separate everytime or reuse?
					if (SUCCEEDED(hr))
					{
						LOG(L"Decoder.GetFrame %u\n", i);
						// Should initialize the converter and create ID2D1Bitmap's to be saved in the std::vector
						hr = formatConverter->Initialize(
							frame,
							GUID_WICPixelFormat32bppPBGRA,
							WICBitmapDitherTypeNone,
							nullptr,
							0.0f,
							WICBitmapPaletteTypeMedianCut
						);
					}

					if (SUCCEEDED(hr))
					{
						LOG(L"FormatConverter initialized %u\n", i);
						hr = renderTarget->CreateBitmapFromWicBitmap(
							formatConverter,
							/*D2D1::BitmapProperties(
								D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT)
							),*/
							&bitmap
						);
					}

					if (SUCCEEDED(hr))
					{
						LOG(L"Created Bitmap from WICBitmap %u\n", i);
						mFrames.emplace_back(std::move(bitmap.Detach()));
					}

				}
				formatConverter.Release();
				if (FAILED(hr))
				{
					LOG(L"Failed to go through all frames\n");
				}
			}
		}
	}


	if (SUCCEEDED(hr))
	{
		hr = mGraphicsFactory.GetWICFactory()->CreateFormatConverter(&formatConverter);
	}

	if (SUCCEEDED(hr))
	{
		LOG(L"Loaded set true\n");
		mLoaded = true;
		mCurrentFrame = mFrames[0];
	}
}

bool AnimatedImage::IsLoaded()
{
	LOG(L"AnimatedImage IsLoaded %d\n", mLoaded);
	return mLoaded;
}

void AnimatedImage::SetLoaded(bool aValue)
{
	if (aValue != mLoaded)
		mLoaded = aValue;
}

void AnimatedImage::Render(ID2D1HwndRenderTarget* renderTarget)
{
	if (!mLoaded)
	{
		return;
	}
	HRESULT hr = S_OK;


	const auto [width, height] = renderTarget->GetSize();

	if (SUCCEEDED(hr))
	{

		if (mCurrentFrame != nullptr)
		{
			// SCALE AND TRANSFORM
			const auto [bitmapWidth, bitmapHeight] = mCurrentFrame->GetSize();

			constexpr float marginLeft = 50.0f;
			constexpr float marginRight = 50.0f;
			constexpr float marginTop = 50.0f;
			constexpr float marginBottom = 50.0f;

			const auto bitmapRatio = mGraphicsFactory.GetRatio(bitmapWidth, bitmapHeight);
			const auto windowRatio = mGraphicsFactory.GetRatio(width - (marginLeft + marginRight), height - (marginTop + marginBottom)); // 100.0f are the imaginary borders, will be moved somewhere

			float scaledWidth;
			float scaledHeight;

			if (bitmapRatio > windowRatio)
			{
				scaledWidth = width - (marginLeft + marginRight);
				scaledHeight = scaledWidth / bitmapRatio;
			}
			else
			{
				scaledHeight = height - (marginTop + marginBottom);
				scaledWidth = scaledHeight * bitmapRatio;
			}

			const auto clientRect = D2D1::RectF(
				(((width - marginLeft) - scaledWidth) / 2.0f),
				(((height - marginTop) - scaledHeight) / 2.0f),
				(((width + marginRight) + scaledWidth) / 2.0f),
				(((height + marginBottom) + scaledHeight) / 2.0f)
			);

			D2D1_MATRIX_3X2_F transform{};

			renderTarget->GetTransform(&transform);
			renderTarget->SetTransform(
				D2D1::Matrix3x2F::Scale(
					1.0,
					1.0,
					D2D1::Point2F(width / 2.0f, height / 2.0f)
				)
			);
			ID2D1SolidColorBrush* brush = nullptr;
			hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Pink), &brush);
			if (SUCCEEDED(hr))
			{
				renderTarget->FillRectangle(clientRect, brush);
			}
			
			auto gridRect = D2D1::RectF(0, 0, width, height);
			renderTarget->DrawBitmap(mFrames[1], gridRect);
			renderTarget->DrawBitmap(mFrames[mFrameCount/2-1], gridRect);
			renderTarget->SetTransform(transform);
			if (brush)
			{
				brush->Release();
				brush = nullptr;
			}
			///
		}
	}

	// here we should draw a picture and then draw it to our HwndRenderTarget
}

void AnimatedImage::Update()
{
	// This should be called in a timer from the Viewer class
}
