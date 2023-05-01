#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wincodec.h>

#include <d2d1.h>
#pragma comment(lib, "d2d1")

#include <type_traits>

#include <string>
constexpr wchar_t PICTURE[] = L"c:\\code\\ArtMesa.png";

ID2D1Factory* g_D2Factory = nullptr;
ID2D1SolidColorBrush* g_Brush = nullptr;
ID2D1Bitmap* g_Bitmap = nullptr;

ID2D1HwndRenderTarget* g_RenderTarget = nullptr;

IWICImagingFactory* g_WICFactory = nullptr;
IWICBitmapDecoder* g_WICBitmapDecoder = nullptr;
IWICBitmapFrameDecode* g_WICFrame = nullptr;
IWICFormatConverter* g_WICConverter = nullptr;
IWICBitmapClipper* g_WICClipper = nullptr;


template<typename T>
void SafeRelease(T* t)
{
	if (t)
	{
		t->Release();
		t = nullptr;
	}
}

LRESULT CALLBACK PictureViewerProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		HRESULT hr = S_OK;
		if (SUCCEEDED(hr))
		{
			hr = g_WICFactory->CreateFormatConverter(&g_WICConverter);
		}
		if (SUCCEEDED(hr))
		{
			hr = g_WICFactory->CreateDecoderFromFilename(PICTURE, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &g_WICBitmapDecoder);
		}
		if (SUCCEEDED(hr))
		{
			hr = g_WICBitmapDecoder->GetFrame(0, &g_WICFrame);
		}
		if (SUCCEEDED(hr))
		{
			hr = g_WICFactory->CreateBitmapClipper(&g_WICClipper);
		}
		WICRect ClipRect{};
		ClipRect.X = 400;
		ClipRect.Y = 300;
		ClipRect.Width = 100;
		ClipRect.Height = 100;
		if (SUCCEEDED(hr))
		{
			hr = g_WICClipper->Initialize(g_WICFrame, &ClipRect);
		}

		if (SUCCEEDED(hr))
		{
			hr = g_WICConverter->Initialize(g_WICClipper, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
		}

		if (SUCCEEDED(hr))
		{
			g_D2Factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(640, 480)), &g_RenderTarget);
		}
		if (SUCCEEDED(hr))
		{
			g_RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &g_Brush);
		}

		if (SUCCEEDED(hr))
		{
			hr = g_RenderTarget->CreateBitmapFromWicBitmap(g_WICConverter, &g_Bitmap);
		}
		if (FAILED(hr))
		{
			OutputDebugStringW(L"Failed to create COM objects\n");
			return 1;
		}
		OutputDebugStringW(L"");
	}
	return 0;
	case WM_SIZE:
	{
		auto Width = LOWORD(lparam);
		auto Height = HIWORD(lparam);
		g_RenderTarget->Resize(D2D1::SizeU(Width, Height));
	}
	return 0;
	case WM_ERASEBKGND:
		return 1;
	case WM_PAINT:
	{
		PAINTSTRUCT ps{};
		BeginPaint(hwnd, &ps);
		g_RenderTarget->BeginDraw();
		g_RenderTarget->SetTransform(D2D1::IdentityMatrix());
		g_RenderTarget->Clear();
		g_RenderTarget->FillRectangle(D2D1::RectF(0.f,0.f,640.f,480.f), g_Brush);
		g_RenderTarget->DrawBitmap(g_Bitmap);
		g_RenderTarget->EndDraw();
		EndPaint(hwnd, &ps);
	}
	return 0;
	case WM_DESTROY:
	{
		SafeRelease(g_WICConverter);
		SafeRelease(g_WICFrame);
		SafeRelease(g_WICBitmapDecoder);
		SafeRelease(g_Brush);
		SafeRelease(g_RenderTarget);


		PostQuitMessage(0);
	}
	return 0;
	default:
		return DefWindowProcW(hwnd, msg, wparam, lparam);
	}
}


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpszCmdLine, _In_ int nCmdShow)
{

	WNDCLASSEX wc{};
	wc.cbClsExtra = wc.cbWndExtra = 0;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpszClassName = L"CPICTUREVIEWER01";
	wc.lpfnWndProc = (WNDPROC)PictureViewerProc;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

	RegisterClassEx(&wc);
	HRESULT hr = S_OK;
	if (SUCCEEDED(hr))
	{
		hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	}

	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_WICFactory));
	}

	if (SUCCEEDED(hr))
	{
		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_D2Factory);
	}

	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to initialize COM\n");
		return 1;

	}



	HWND hwnd = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		wc.lpszClassName,
		L"",
		0,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		nullptr, nullptr,
		hInstance,
		nullptr);

	UpdateWindow(hwnd);
	ShowWindow(hwnd, nCmdShow);

	MSG msg{};
	while (GetMessage(&msg, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	SafeRelease(g_D2Factory);
	SafeRelease(g_WICFactory);
	CoUninitialize();

	return (int)msg.wParam;
}