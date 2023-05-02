#pragma once
#include "Application.h"
#include <dwrite.h>
constexpr wchar_t PICTURE[] = L"C:\\Code\\ArtMesa.png";

template<typename T>
inline void SafeRelease(T*& t)
{
	if (NULL != t)
	{
		t->Release();
		t = nullptr;
	}
}
enum class MouseMoveControl : UINT32
{
	Ctrl = 0x0008,
	LeftButton = 0x0001,
	MiddleButton = 0x0010,
	RightButton = 0x0002,
	Shift = 0x0004,
	XButton1 = 0x0020,
	XButton2 = 0x0040,
};

class Viewer
{
public:
	Viewer();
	~Viewer();

	HRESULT Initialize(HINSTANCE hInst);
	HRESULT CreateDeviceResources(HWND hwnd);
	LRESULT OnPaint(HWND hwnd);
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	HRESULT LoadFile(std::wstring const& Path);

	virtual LRESULT OnNcCreate(WPARAM wparam, LPARAM lparam);

	virtual void OnMouseMove(MouseMoveControl ctrl, int x, int y);

	static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (msg == WM_NCCREATE)
		{
			OutputDebugStringW(L"Received WM_NCCREATE\n");
			auto cs = reinterpret_cast<LPCREATESTRUCT>(lparam);
			Viewer* pThis = reinterpret_cast<Viewer*>(cs->lpCreateParams);
			pThis->m_hwnd = hwnd;
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			return pThis->OnNcCreate(wparam, lparam);
		}
		else if (Viewer* pThis = reinterpret_cast<Viewer*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA)))
		{
			return pThis->WndProc(hwnd, msg, wparam, lparam);
		}
		return DefWindowProcW(hwnd, msg, wparam, lparam);
	}
private:
	HINSTANCE m_hInst{};
	HWND m_hwnd{};
	IWICImagingFactory* m_wic_factory = nullptr;
	IWICFormatConverter* m_wic_converter = nullptr;

	ID2D1Factory* m_d2_factory = nullptr;
	ID2D1HwndRenderTarget* m_renderTarget = nullptr;
	ID2D1Bitmap* m_bitmap = nullptr;
	ID2D1SolidColorBrush* m_brush = nullptr;

	IDWriteFactory* m_dwrite_factory = nullptr;

	float m_lastMouseX{};
	float m_lastMouseY{};
	float m_imageX{};
	float m_imageY{};
	float m_scaleFactor = 1.0;

};

