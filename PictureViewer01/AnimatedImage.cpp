#include "AnimatedImage.h"
#include "GraphicFactory.h"
#include "GraphicsManager.h"
#include "Log.h"
#include <stdexcept>
#include <atlbase.h>


namespace
{
	UINT GetWidth(IWICMetadataQueryReader* metaReader)
	{
		PROPVARIANT propValue;
		PropVariantInit(&propValue);

		UINT imageWidth{};


		HRESULT hr = metaReader->GetMetadataByName(L"/logscrdesc/Width", &propValue);

		if (SUCCEEDED(hr))
		{
			hr = propValue.vt != VT_UI2 ? E_FAIL : S_OK;

		}

		if (SUCCEEDED(hr))
		{
			imageWidth = propValue.uiVal;
			PropVariantClear(&propValue);
		}

		return imageWidth;
	}

	UINT GetHeight(IWICMetadataQueryReader* metaReader)
	{
		PROPVARIANT propValue;
		PropVariantInit(&propValue);

		UINT imageHeight{};
		HRESULT hr = metaReader->GetMetadataByName(L"/logscrdesc/Height", &propValue);
		if (SUCCEEDED(hr))
		{
			hr = propValue.vt != VT_UI2 ? E_FAIL : S_OK;
		}

		if (SUCCEEDED(hr))
		{
			imageHeight = propValue.uiVal;
			PropVariantClear(&propValue);
		}

		return imageHeight;
	}

	struct Transparency
	{
		

		int Index;
		bool Transparent;
		static Transparency TransparencyInformation(IWICMetadataQueryReader* metaReader, size_t actualColors)
		{
			Transparency trans{};
			PROPVARIANT propValue;
			PropVariantInit(&propValue);
			HRESULT hr = metaReader->GetMetadataByName(L"/grctlext/TransparencyFlag", &propValue);
			if (SUCCEEDED(hr))
			{
				hr = propValue.vt == VT_BOOL ? S_OK : E_FAIL;
				trans.Transparent = propValue.boolVal;
				if (SUCCEEDED(hr) && propValue.boolVal)
				{
					PropVariantClear(&propValue);
					hr = metaReader->GetMetadataByName(L"/grctlext/TransparentColorIndex", &propValue);
					if (SUCCEEDED(hr) && propValue.uiVal < actualColors)
					{
						trans.Index = static_cast<int>(propValue.uiVal);
					}
				}
			}
			PropVariantClear(&propValue);
			return trans;
		}

		bool operator==(const Transparency& other) const = default;
	};



}
void AnimatedImage::DrawBackground(ID2D1HwndRenderTarget* renderTarget)
{
	HRESULT hr = S_OK;

	CComPtr<ID2D1SolidColorBrush> brush;
	const auto [width, height] = renderTarget->GetSize();

	auto clientRect = D2D1::RectF(0, 0, width, height);
	hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Pink), &brush);
	if (SUCCEEDED(hr))
	{
		renderTarget->FillRectangle(clientRect, brush);
	}

}
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


// Load
// Figured out how this should be implemented in a naive solution.
// We need to load all frames, and based on each frame we do a "blend" so we remove and add pixels from the current frame on the previous frame
// Then we save the Bitmap and we play up the the images as if its a animation in a book.
// 
void
AnimatedImage::Load(
	std::wstring const& filepath,
	ID2D1HwndRenderTarget* renderTarget
)
{
	HRESULT hr = S_OK;

	CComPtr<IWICBitmapDecoder> decoder;
	

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


	{
		GUID containerFormat;
		hr = decoder->GetContainerFormat(&containerFormat);
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to get container format.");
		}

		if (std::memcmp(&containerFormat, &GUID_ContainerFormatGif, sizeof(GUID) != 0))
		{
			throw std::runtime_error("Format not supported.");
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = decoder->GetFrameCount(&mFrameCount);
	}

	if (SUCCEEDED(hr))
	{
		LOG(L"Framecount %u\n", mFrameCount);
	}

	CComPtr<IWICMetadataQueryReader> metaReader;
	hr = decoder->GetMetadataQueryReader(&metaReader);

	UINT ImageWidth = GetWidth(metaReader);
	UINT ImageHeight = GetHeight(metaReader);
	LOG(L"Image: %u,%u\n", ImageWidth, ImageHeight);

	for (unsigned i = 0; i < mFrameCount; ++i)
	{
		CComPtr<IWICFormatConverter> formatConverter;
		CComPtr<IWICBitmapFrameDecode> frame;
		CComPtr<ID2D1Bitmap> bitmap;

		hr = decoder->GetFrame(i, &frame);

		WICPixelFormatGUID format;
		if (SUCCEEDED(hr))
		{
			hr = frame->GetPixelFormat(&format);
		}

		WICColor rgbColors[256] = {};
		UINT actualColors = 0;

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

		LOG(L"Got the palette\n");
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
				hr = renderTarget->CreateBitmapFromWicBitmap(
					formatConverter,
					&bitmap
				);
			}

			if (SUCCEEDED(hr))
			{
				LOG(L"Created Bitmap from WICBitmap %u\n", i);
				mFrames.emplace_back(std::move(bitmap.Detach()));
			}

		}
		if (FAILED(hr))
		{
			LOG(L"Failed to go through all frames\n");
		}
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
			const auto windowRatio = mGraphicsFactory.GetRatio(
				width - (marginLeft + marginRight), 
				height - (marginTop + marginBottom)
			); // 100.0f are the imaginary borders, will be moved somewhere

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
			DrawBackground(renderTarget);
			
			auto gridRect = D2D1::RectF(0, 0, mFrames[0]->GetSize().width, mFrames[0]->GetSize().height);
			renderTarget->DrawBitmap(mFrames[0], gridRect);
			gridRect.left += mFrames[0]->GetSize().width;
			gridRect.right += mFrames[0]->GetSize().width;
			renderTarget->DrawBitmap(mFrames[1], gridRect);

			renderTarget->SetTransform(transform);
		}
	}

	// here we should draw a picture and then draw it to our HwndRenderTarget
}

void AnimatedImage::Update()
{
	// This should be called in a timer from the Viewer class
}
