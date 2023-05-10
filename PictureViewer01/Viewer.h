#pragma once
#include "Application.h"
#include "SafeRelease.h"
#include "BaseWindow.h"

#include <memory>

#include <dwrite.h>
#include <commdlg.h>

constexpr wchar_t PICTURE[] = L"C:\\Code\\digits.png";



struct ZipMemory
{
	void* GlobalData;
	HGLOBAL Global;

};

struct ZipFile
{
	std::string Name;
	size_t Size;
	void* GlobalData;
	HGLOBAL Global;
	IStream* Stream;

	ZipFile(std::string const name, size_t size) :
		Name(name),
		Size(size),
		GlobalData(nullptr),
		Global(nullptr),
		Stream(nullptr)
	{

	}

	~ZipFile()
	{
		OutputDebugStringW(L"ZipFile destructor\n");
		if (Global)
			GlobalFree(Global);
		SafeRelease(Stream);
	}

	HRESULT Write(std::vector<byte>&& Bytes)
	{
		HRESULT hr = S_OK;
		if (SUCCEEDED(hr))
		{
			Global = GlobalAlloc(GMEM_MOVEABLE, Bytes.size());
			hr = Global ? S_OK : E_FAIL;
		}
		if (SUCCEEDED(hr))
		{
			GlobalData = GlobalLock(Global);
			hr = GlobalData ? S_OK : E_FAIL;
		}

		if (SUCCEEDED(hr))
		{
			CopyMemory(GlobalData, Bytes.data(), Bytes.size());
			hr = GlobalUnlock(GlobalData) ? S_OK : E_FAIL;
		}

		return hr;
	}

	HRESULT RecreateStream()
	{
		SafeRelease(Stream);
		return CreateStreamOnHGlobal(Global, false, &Stream);
	}
};

struct ZipList
{
	std::vector<ZipFile> m_list;
	std::vector<ZipMemory> m_memory_list;


};
std::wstring ToWideString(std::string const& Text);
std::string FromWideString(std::wstring const& Text);
std::vector<std::shared_ptr<ZipFile>> ReadZip(std::wstring const& Filename);
class GraphicManager
{
	IWICImagingFactory* m_wic_factory = nullptr;
	IWICFormatConverter* m_wic_converter = nullptr;

	ID2D1Factory* m_d2_factory = nullptr;
	ID2D1HwndRenderTarget* m_renderTarget = nullptr;
	ID2D1Bitmap* m_bitmap = nullptr;
	ID2D1SolidColorBrush* m_brush = nullptr;

	IDWriteFactory* m_dwrite_factory = nullptr;
	IDWriteTextFormat* m_textFormat = nullptr;
	HWND mHwnd;
public:
	explicit GraphicManager(HWND hwnd) : mHwnd(hwnd)
	{

	}
	~GraphicManager()
	{
		SafeRelease(m_brush);
		SafeRelease(m_bitmap);
		SafeRelease(m_d2_factory);
		SafeRelease(m_renderTarget);

		SafeRelease(m_wic_factory);
		SafeRelease(m_wic_converter);

		SafeRelease(m_textFormat);
		SafeRelease(m_dwrite_factory);
	}


	GraphicManager() :m_bitmap(nullptr),
		m_wic_factory(nullptr),
		m_d2_factory(nullptr),
		m_brush(nullptr),
		m_renderTarget(nullptr),
		mHwnd(nullptr)
	{
		
	}


	void Initialize(HWND hwnd)
	{
		mHwnd = hwnd;
		HRESULT hr = S_OK;
		if (SUCCEEDED(hr))
		{
			hr = CoCreateInstance(
				CLSID_WICImagingFactory,
				nullptr,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&m_wic_factory)
			);
		}

		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Created WIC Factory\n");
			hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_d2_factory);
		}

		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Created D2D1 Factory\n");
			hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, _uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_dwrite_factory));
		}

		if (SUCCEEDED(hr))
		{
			hr = m_dwrite_factory->CreateTextFormat(
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
	D2D1_WINDOW_STATE CheckWindowState() const
	{
		return m_renderTarget->CheckWindowState();
	}

	ID2D1HwndRenderTarget* RenderTarget()
	{
		if (m_renderTarget != nullptr)
		{
			return m_renderTarget;
		}
		return nullptr;
		
	}
	ID2D1Bitmap* Bitmap()
	{
		return m_bitmap;
	}

	IDWriteTextFormat* TextFormat()
	{
		return m_textFormat;
	}
	IWICFormatConverter* Converter()
	{
		return m_wic_converter;
	}

	IDWriteFactory* WriteFactory()
	{
		return m_dwrite_factory;
	}

	IWICImagingFactory* WICFactory()
	{
		return m_wic_factory;
	}
	HRESULT CreateFormatConverter()
	{
		ReleaseConverter();
		HRESULT hr = m_wic_factory->CreateFormatConverter(&m_wic_converter);
		return hr;
	}
	void ReleaseConverter()
	{
		SafeRelease(m_wic_converter);
	}
	void ReleaseDeviceResources()
	{
		SafeRelease(m_brush);
		SafeRelease(m_bitmap);
		SafeRelease(m_renderTarget);
	}
	void ReleaseBitmap()
	{
		SafeRelease(m_bitmap);
	}

	HRESULT CreateBitmapFromWicBitmap()
	{
		ReleaseBitmap();
		return m_renderTarget->CreateBitmapFromWicBitmap(m_wic_converter, &m_bitmap);
	}

	ID2D1SolidColorBrush* Brush()
	{
		return m_brush;
	}

	HRESULT CreateDeviceResources(HWND hwnd)
	{
		HRESULT hr = S_OK;

		if (!m_renderTarget)
		{
			RECT rc{};
			hr = GetClientRect(hwnd, &rc) ? S_OK : E_FAIL;
			if (SUCCEEDED(hr))
			{
				OutputDebugStringW(L"Creating HWND render target\n");
				D2D1_RENDER_TARGET_PROPERTIES renderProperties = D2D1::RenderTargetProperties();
				renderProperties.dpiX = 96.0f;
				renderProperties.dpiY = 96.0f;

				auto hwndProperties = D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU((rc.right - rc.left), (rc.bottom - rc.top)));

				hr = m_d2_factory->CreateHwndRenderTarget(
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
};
class Viewer : public BaseWindow<Viewer>
{
public:
	Viewer();
	~Viewer();

	HRESULT Initialize(HINSTANCE hInst);
	HRESULT LoadFile(std::wstring const& Path);
	HRESULT LoadImage(int delta);
	HRESULT OpenArchive();

	virtual void OnSize(UINT Width, UINT Height) noexcept override;
	virtual LRESULT OnPaint(HWND hwnd) noexcept override;
	virtual void OnKeyDown(UINT32 VirtualKey) noexcept override;
	virtual void OnMouseMove(MouseMoveControl ctrl, float x, float y) noexcept override;
	virtual void OnLButtonDown(float x, float y) noexcept override;
	virtual void OnLButtonUp(float x, float y) noexcept override;
	virtual void OnMouseScrollWheel(short delta) noexcept override;

	void Start();
private:
	std::unique_ptr<GraphicManager> mGraphicManager;

	float m_lastMouseX{};
	float m_lastMouseY{};
	float m_imageX{};
	float m_imageY{};
	float m_scaleFactor = 1.0;

	int m_currentPage = -1;
	std::vector<std::shared_ptr<ZipFile>> m_zip_files;
	std::vector<HGLOBAL> m_zip_globals;

};

