#include "AnimatedImage.h"
#include "GraphicFactory.h"
#include "GraphicsManager.h"
#include "Log.h"
#include <stdexcept>

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
	LOG(L"Framecount %u\n", mFrameCount);

	IWICFormatConverter* formatConverter = nullptr;
	if (SUCCEEDED(hr))
	{
		HRESULT hr = S_OK;
		

		if (SUCCEEDED(hr))
		{
			for (unsigned i = 0; i < mFrameCount; i++)
			{

				IWICBitmapFrameDecode* frame = nullptr;
				ID2D1Bitmap* bitmap = nullptr;
				if (SUCCEEDED(hr))
				{
					hr = decoder->GetFrame(i, &frame);
				}
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
							WICBitmapPaletteTypeCustom
						);
					}
					if (SUCCEEDED(hr))
					{
						LOG(L"FormatConverter initialized %u\n", i);
						hr = renderTarget->CreateBitmapFromWicBitmap(formatConverter, &bitmap);
					}

					if (SUCCEEDED(hr))
					{
						LOG(L"Created Bitmap from WICBitmap %u\n", i);
						mFrames.emplace_back(std::move(bitmap));
					}

				}

				if (FAILED(hr))
				{
					LOG(L"Failed to go through all frames\n");
				}

				frame->Release();
				frame = nullptr;
				formatConverter->Release();
				formatConverter = nullptr;
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


	if (decoder)
	{
		decoder->Release();
		decoder = nullptr;
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
	HRESULT hr = S_OK;


	const auto [width, height] = renderTarget->GetSize();

	if (SUCCEEDED(hr))
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
		renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush);
		renderTarget->FillRectangle(clientRect, brush);
		renderTarget->DrawBitmap(mCurrentFrame, clientRect);
		renderTarget->SetTransform(transform);
		if (brush)
		{
			brush->Release();
			brush = nullptr;
		}
		///
	}

	// here we should draw a picture and then draw it to our HwndRenderTarget
}

void AnimatedImage::Update(ID2D1HwndRenderTarget* renderTarget)
{
	mCurrentFrame = mFrames.at(mCurrentFrameIdx++ % mFrameCount);
	InvalidateRect(renderTarget->GetHwnd(), nullptr, TRUE);

	// This should be called in a timer from the Viewer class
}
