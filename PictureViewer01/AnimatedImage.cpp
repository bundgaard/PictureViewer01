#include "AnimatedImage.h"
#include "GraphicFactory.h"
#include "GraphicsManager.h"

LRESULT AnimatedImage::sWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	AnimatedImage* clz = (AnimatedImage*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

	switch (msg)
	{
	case WM_CREATE:
	{
		SetTimer(hwnd, 0, 16, nullptr);
		return 0;
	}
	case WM_TIMER:
	{
		OutputDebugStringW(L"Timer\n");
		InvalidateRect(hwnd, nullptr, true);
		return 0;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		TextOutW(hdc, 0, 0, L"Hello World", 11);
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_DESTROY:
	{
		OutputDebugStringW(L"AnimatedImage Destroy msg\n");
		KillTimer(hwnd, 0);
		DestroyWindow(hwnd);
		return 0;
	}
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

AnimatedImage::AnimatedImage(const HWND Parent, GraphicFactory& graphicsFactory)
	: mGraphicsFactory(graphicsFactory)
	, mGraphicsManager(std::make_unique<GraphicsManager>(mGraphicsFactory))
	, mFrameCount(0)
	, mWnd(nullptr)
{
	HRESULT hr = S_OK;

	WNDCLASSEX wc{};
	wc.cbSize = sizeof(wc);
	wc.hInstance = GetModuleHandleW(0);
	wc.lpszClassName = L"ANIMATEDIMAGE01WND";
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpfnWndProc = (WNDPROC)sWndProc;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	hr = RegisterClassEx(&wc) ? S_OK : E_FAIL;

	if (SUCCEEDED(hr))
	{
		mWnd = CreateWindowExW(
			WS_EX_OVERLAPPEDWINDOW,
			wc.lpszClassName,
			L"",
			WS_OVERLAPPEDWINDOW,
			0, 0,
			200, 200,
			Parent,
			nullptr,
			wc.hInstance,
			this);
		hr = mWnd ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		mGraphicsManager->Initialize(mWnd);
	}

}

AnimatedImage::~AnimatedImage()
{
	KillTimer(mWnd, 0);
	DestroyWindow(mWnd);
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

	ShowWindow(mWnd, SW_SHOW);
}
