#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wincodec.h>

#include <d2d1.h>
#pragma comment(lib, "d2d1")

#include <type_traits>

#include <string>
#include <atlbase.h>
#include <memory>

constexpr wchar_t PICTURE[] = L"c:\\code\\ArtMesa.png";
CComPtr<ID2D1Factory> g_D2Factory;
CComPtr<IWICImagingFactory> g_WICFactory;
CComPtr<ID2D1SolidColorBrush> g_Brush;
CComPtr<ID2D1Bitmap> g_Bitmap;

template<typename T>
inline void SafeRelease(T* t)
{
	if (t)
	{
		t->Release();
		t = nullptr;
	}
}


class Viewer
{
public:
	Viewer()
	{}

	~Viewer()
	{
		SafeRelease(m_wic_factory);
		SafeRelease(m_d2_factory);
		SafeRelease(m_renderTarget);
		SafeRelease(m_bitmap);
		SafeRelease(m_wic_converter);

	}
	HRESULT Initialize(HINSTANCE hInst)
	{
		WNDCLASSEX wc{};
		HRESULT hr = S_OK;

		if (SUCCEEDED(hr))
		{
			hr = CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_WICFactory));
		}

		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Created WIC Factory\n");
			hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_D2Factory);
		}

		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Created D2D1 Factory\n");
			wc.cbSize = sizeof(WNDCLASSEX);
			wc.cbWndExtra = sizeof(LONG_PTR);
			wc.cbClsExtra = 0;


			wc.lpszClassName = L"CPICTUREVIEWER01";
			wc.lpfnWndProc = (WNDPROC)Viewer::s_WndProc;
			wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
			wc.style = CS_HREDRAW | CS_VREDRAW;
			wc.hInstance = hInst;

			m_hInst = hInst;

			hr = RegisterClassEx(&wc) ? S_OK : E_FAIL;
		}

		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Created class\n");
			HWND hwnd = CreateWindowW(
				L"CPICTUREVIEWER01",
				L"VIEWER",
				WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT,
				640, 480,
				nullptr, nullptr,
				m_hInst,
				this);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, PtrToLong(this));
			OutputDebugStringW(L"Creating Window\n");
			
			hr = hwnd ? S_OK : E_FAIL;

			UpdateWindow(hwnd);
			ShowWindow(hwnd, SW_SHOW);
		}
		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Created Window\n");
		}
		return hr;
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
				hr = g_D2Factory->CreateHwndRenderTarget(
					D2D1::RenderTargetProperties(),
					D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(640, 480)),
					&m_renderTarget);

			}
		}

		return hr;
	}

	LRESULT OnPaint(HWND hwnd)
	{
		HRESULT hr = S_OK;
		PAINTSTRUCT ps{};
		OutputDebugStringW(L"Received WM_PAINT\n");
		if (BeginPaint(hwnd, &ps))
		{
			hr = CreateDeviceResources(hwnd);

			if (SUCCEEDED(hr) && !(m_renderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
			{
				m_renderTarget->BeginDraw();
				m_renderTarget->SetTransform(D2D1::IdentityMatrix());
				m_renderTarget->Clear();
				m_renderTarget->FillRectangle(D2D1::RectF(0.f, 0.f, 640.f, 480.f), g_Brush);
				m_renderTarget->DrawBitmap(g_Bitmap);
				hr = m_renderTarget->EndDraw();
				if (hr == D2DERR_RECREATE_TARGET)
				{
					SafeRelease(m_bitmap);
					SafeRelease(m_renderTarget);

					hr = InvalidateRect(hwnd, nullptr, true) ? S_OK : E_FAIL;
				}
			}
			EndPaint(hwnd, &ps);
		}
		return SUCCEEDED(hr) ? S_OK : E_FAIL;
	}

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		switch (msg)
		{
		case WM_PAINT:
			return OnPaint(hwnd);
		case WM_SIZE:
		{
			OutputDebugStringW(L"Received WM_SIZE\n");
			auto Width = LOWORD(lparam);
			auto Height = HIWORD(lparam);
			wchar_t Buf[64] = { 0 };
			swprintf(Buf, 64, L"%d,%d\n", Width, Height);
			OutputDebugStringW(Buf);
			if (m_renderTarget != nullptr)
			{
				if (FAILED(m_renderTarget->Resize(D2D1::SizeU(Width, Height))))
				{
					SafeRelease(m_renderTarget);
					SafeRelease(m_bitmap);
				}
			}
		}
		return 0;
		default:
			return DefWindowProcW(hwnd, msg, wparam, lparam);
		}
	}


	static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		Viewer* pThis;
		LRESULT lRet{};
		if (msg == WM_NCCREATE)
		{
			OutputDebugStringW(L"Received WM_NCCREATE\n");
			auto cs = reinterpret_cast<CREATESTRUCT*>(lparam);
			pThis = reinterpret_cast<Viewer*>(cs->lpCreateParams);
			pThis->m_hInst = GetModuleHandleW(0);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, PtrToLong(pThis));
			lRet = DefWindowProcW(hwnd, msg, wparam, lparam);
		}
		else
		{
			pThis = reinterpret_cast<Viewer*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
			if (pThis)
			{
				lRet = pThis->WndProc(hwnd, msg, wparam, lparam);
			}
			else
			{
				lRet = DefWindowProcW(hwnd, msg, wparam, lparam);
			}
		}
		return lRet;
	}
private:
	HINSTANCE m_hInst;
	IWICImagingFactory* m_wic_factory;
	ID2D1Factory* m_d2_factory;
	ID2D1HwndRenderTarget* m_renderTarget;
	ID2D1Bitmap* m_bitmap;
	IWICFormatConverter* m_wic_converter;
};

class Image
{
	CComPtr<IWICBitmapDecoder> m_bitmapdecoder;

	CComPtr<IWICFormatConverter> m_converter;
	CComPtr<IWICBitmapClipper> m_clipper;

	std::wstring m_filename;

public:
	Image(std::wstring const& Filename, IWICImagingFactory* pWICFactory) : m_filename(Filename)
	{
		HRESULT hr = S_OK;
		CComPtr<IWICBitmapFrameDecode> m_frame;
		if (SUCCEEDED(hr))
		{
			hr = pWICFactory->CreateDecoderFromFilename(Filename.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &m_bitmapdecoder);
		}

		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Created decoder from filename.\n");
			hr = pWICFactory->CreateFormatConverter(&m_converter);
		}

		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Created format converter.\n");
			hr = m_bitmapdecoder->GetFrame(0, &m_frame);
		}

		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Retrieved frame from file.\n");
			hr = m_converter->Initialize(m_frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
		}
		if (SUCCEEDED(hr))
		{
			m_bitmapdecoder.Release();
		}
		if (FAILED(hr))
		{

			OutputDebugStringW(L"Failed to initialize Image\n");
		}

	}

	// Up to receiver to release.
	IWICBitmapSource* Bitmap()
	{
		return m_converter.Detach();
	}

};

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpszCmdLine, _In_ int nCmdShow)
{
	HRESULT hr = S_OK;
	if (SUCCEEDED(hr))
	{
		hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

		if (SUCCEEDED(hr))
		{
			Viewer app;
			hr = app.Initialize(hInstance);
			if (SUCCEEDED(hr))
			{
				OutputDebugStringW(L"Initialized Viewer\n");
				MSG msg{};
				while (GetMessage(&msg, nullptr, 0, 0) != 0)
				{
					TranslateMessage(&msg);
					DispatchMessageW(&msg);
				}

				CoUninitialize();
			}
		}
	}

	return 0;
}