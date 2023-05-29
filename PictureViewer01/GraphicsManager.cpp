#include "GraphicsManager.h"
#include <wincodec.h>
#include <dwrite.h>
#include <string>
#include "Log.h"
#include "GraphicFactory.h"
GraphicsManager::~GraphicsManager()
{
	LOG(L"GraphicManager DTOR\n");
	SafeRelease(m_brush);
	SafeRelease(m_bitmap);
	SafeRelease(m_renderTarget);
	SafeRelease(m_wic_converter);
	SafeRelease(m_textFormat);
}

GraphicsManager::GraphicsManager(GraphicFactory& factory) 
	: m_bitmap(nullptr)
	, mGraphicFactory(factory)
	, m_brush(nullptr)
	, m_renderTarget(nullptr)
	, mHwnd(nullptr)
{

}

void GraphicsManager::Initialize(HWND hwnd)
{
	mHwnd = hwnd;
	LOG(L"GraphicManager set HWND\n");
	HRESULT hr = S_OK;
	
	if (SUCCEEDED(hr))
	{
		hr = mGraphicFactory.GetWriteFactory()->CreateTextFormat(
			L"Comic Sans MS",
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			18.0f,
			L"",
			&m_textFormat);
	}
}

D2D1_WINDOW_STATE GraphicsManager::CheckWindowState() const
{
	return m_renderTarget->CheckWindowState();
}

ID2D1HwndRenderTarget* GraphicsManager::RenderTarget()
{
	if (m_renderTarget != nullptr)
	{
		return m_renderTarget;
	}
	return nullptr;

}

ID2D1Bitmap* GraphicsManager::Bitmap()
{
	return m_bitmap;
}

IDWriteTextFormat* GraphicsManager::TextFormat()
{
	return m_textFormat;
}

IWICFormatConverter* GraphicsManager::Converter()
{
	return m_wic_converter;
}

HRESULT GraphicsManager::CreateFormatConverter()
{
	ReleaseConverter();
	HRESULT hr = mGraphicFactory.GetWICFactory()->CreateFormatConverter(&m_wic_converter);
	return hr;
}

void GraphicsManager::ReleaseConverter()
{
	SafeRelease(m_wic_converter);
}

void GraphicsManager::ReleaseDeviceResources()
{
	SafeRelease(m_brush);
	SafeRelease(m_bitmap);
	SafeRelease(m_renderTarget);
}

void GraphicsManager::ReleaseBitmap()
{
	SafeRelease(m_bitmap);
}

HRESULT GraphicsManager::CreateBitmapFromWicBitmap()
{
	ReleaseBitmap();
	return m_renderTarget->CreateBitmapFromWicBitmap(m_wic_converter, &m_bitmap);
}

ID2D1SolidColorBrush* GraphicsManager::Brush()
{
	return m_brush;
}

HRESULT GraphicsManager::CreateDeviceResources(HWND hwnd)
{
	HRESULT hr = S_OK;
	hr = hwnd ? S_OK : E_FAIL;
	if (!m_renderTarget && hwnd != nullptr)
	{
		RECT rc{};
		hr = GetClientRect(hwnd, &rc) ? S_OK : E_FAIL;
		if (SUCCEEDED(hr))
		{
			LOG(L"Creating HWND render target\n");
			D2D1_RENDER_TARGET_PROPERTIES renderProperties = D2D1::RenderTargetProperties();
			renderProperties.dpiX = 96.0f;
			renderProperties.dpiY = 96.0f;

			auto hwndProperties = D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU((rc.right - rc.left), (rc.bottom - rc.top)));

			hr = mGraphicFactory.GetD2Factory()->CreateHwndRenderTarget(
				renderProperties,
				hwndProperties,
				&m_renderTarget);
			if (SUCCEEDED(hr))
			{
				hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_brush);
			}
		}
	}

	return hr;
}


HRESULT GraphicsManager::CreateBitmapFromIStream(IStream* pStream)
{
	HRESULT hr = S_OK;
	IWICBitmapDecoder* decoder = nullptr;

	ReleaseBitmap();
	ReleaseConverter();

	hr = mGraphicFactory.GetWICFactory()->CreateDecoderFromStream(
		pStream,
		nullptr,
		WICDecodeMetadataCacheOnDemand,
		&decoder);

	if (SUCCEEDED(hr))
	{
		unsigned int frameCount{};
		hr = decoder->GetFrameCount(&frameCount);
		LOG(L"Frames %d\n", frameCount);
	}

	IWICBitmapFrameDecode* frame = nullptr;
	if (SUCCEEDED(hr))
	{
		LOG(L"Created decoder from Stream\n");
		hr = decoder->GetFrame(0, &frame);
	}

	if (SUCCEEDED(hr))
	{
		LOG(L"GetFrame\n");
		CreateFormatConverter();
	}

	if (SUCCEEDED(hr))
	{
		LOG(L"Create format converter\n");
		hr = Converter()->Initialize(
			frame, 
			GUID_WICPixelFormat32bppPBGRA, 
			WICBitmapDitherTypeNone, 
			nullptr, 
			0.0f, 
			WICBitmapPaletteTypeCustom);
	}

	if (SUCCEEDED(hr))
	{
		LOG(L"Initialized converter\n");
		hr = CreateDeviceResources(mHwnd);
	}

	if (SUCCEEDED(hr))
	{
		LOG(L"Create device resources\n");
		CreateBitmapFromWicBitmap();
	}

	SafeRelease(decoder);
	SafeRelease(frame);
	return hr;
}

HRESULT GraphicsManager::CreateBitmapFromFile(std::wstring const& Filepath)
{
	HRESULT hr = S_OK;
	IWICBitmapDecoder* decoder = nullptr;
	if (SUCCEEDED(hr))
	{
		hr = mGraphicFactory.GetWICFactory()->CreateDecoderFromFilename(
			Filepath.c_str(), 
			nullptr, 
			GENERIC_READ, 
			WICDecodeMetadataCacheOnDemand, 
			&decoder);
	}

	IWICBitmapFrameDecode* frame = nullptr;
	if (SUCCEEDED(hr))
	{
		hr = decoder->GetFrame(0, &frame);
	}

	if (SUCCEEDED(hr))
	{

		hr = CreateFormatConverter();
	}

	if (SUCCEEDED(hr))
	{
		hr = Converter()->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
	}

	if (SUCCEEDED(hr))
	{
		hr = CreateDeviceResources(mHwnd);
	}

	if (SUCCEEDED(hr))
	{
		hr = CreateBitmapFromWicBitmap();
	}

	SafeRelease(decoder);
	SafeRelease(frame);
	return hr;
}

void GraphicsManager::Resize(int Width, int Height)
{
	if (RenderTarget() != nullptr)
	{
		if (FAILED(RenderTarget()->Resize(D2D1::SizeU(Width, Height))))
		{
			ReleaseDeviceResources();
		}
	}
}

void GraphicsManager::DrawText(std::wstring const& Text, float x, float y, D2D1::ColorF color)
{
	HRESULT hr = S_OK;
	IDWriteTextLayout* layout = nullptr;
	D2D1_SIZE_F Size = RenderTarget()->GetSize();

	if (SUCCEEDED(hr))
	{
		hr = mGraphicFactory.GetWriteFactory()->CreateTextLayout(Text.c_str(), static_cast<UINT32>(Text.size()), TextFormat(), Size.width, Size.height, &layout);
	}
	
	DWRITE_TEXT_METRICS metric{};
	if (SUCCEEDED(hr))
	{
		hr = layout->GetMetrics(&metric);
	}

	if (SUCCEEDED(hr))
	{
		auto oldColor = Brush()->GetColor();
		Brush()->SetColor(color);
		RenderTarget()->DrawTextLayout(D2D1::Point2F(x, y), layout, Brush(), D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
		Brush()->SetColor(oldColor);
	}
	SafeRelease(layout);
}

void GraphicsManager::DrawTextCentered(std::wstring const& Text, float y, D2D1::ColorF color)
{
	HRESULT hr = S_OK;
	IDWriteTextLayout* layout = nullptr;
	D2D1_SIZE_F Size = RenderTarget()->GetSize();

	if (SUCCEEDED(hr))
	{
		hr = mGraphicFactory.GetWriteFactory()->CreateTextLayout(Text.c_str(), static_cast<UINT32>(Text.size()), TextFormat(), Size.width, Size.height, &layout);
	}

	DWRITE_TEXT_METRICS metric{};
	if (SUCCEEDED(hr))
	{
		hr = layout->GetMetrics(&metric);
	}

	if (SUCCEEDED(hr))
	{
		auto oldColor = Brush()->GetColor();
		Brush()->SetColor(color);
		RenderTarget()->DrawTextLayout(D2D1::Point2F((Size.width - metric.width) / 2.0f, y), layout, Brush(), D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
		Brush()->SetColor(oldColor);
	}
	SafeRelease(layout);

}
