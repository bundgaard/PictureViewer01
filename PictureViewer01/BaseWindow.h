#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>

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

template<typename Clz>
class BaseWindow
{

public:
	static Clz* Handle(HWND hwnd) noexcept
	{
		return reinterpret_cast<Clz*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
	}

	virtual ~BaseWindow() = 0;
	virtual HRESULT Initialize(HINSTANCE hInst) = 0;

	[[nodiscard]] virtual LRESULT OnPaint(HWND hwnd) = 0;


	virtual void OnSize(UINT Width, UINT Height) = 0;
	virtual void OnLButtonDown(float x, float y) noexcept = 0;
	virtual void OnLButtonUp(float x, float y) noexcept = 0;
	virtual void OnMouseMove(MouseMoveControl ctrl, float x, float y) noexcept = 0;
	virtual void OnMouseScrollWheel(short delta) noexcept = 0;
	virtual void OnKeyDown(UINT32 VirtualKey) noexcept = 0;


	[[nodiscard]] virtual LRESULT OnNcCreate(WPARAM wparam, LPARAM lparam) noexcept
	{
		OutputDebugStringW(L"In OnNcCreate\n");
		SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		return DefWindowProcW(m_hwnd, WM_NCCREATE, wparam, lparam);
	}
	[[nodiscard]] virtual LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept
	{
		switch (msg)
		{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		case WM_KEYDOWN:
		{
			OnKeyDown(static_cast<UINT32>(wparam));
			break;
		}
		case WM_LBUTTONDOWN:
		{
			OnLButtonDown(
				static_cast<float>(GET_X_LPARAM(lparam)),
				static_cast<float>(GET_Y_LPARAM(lparam)));
			break;
		}
		case WM_LBUTTONUP:
		{
			OnLButtonUp(static_cast<float>(GET_X_LPARAM(lparam)),
				static_cast<float>(GET_Y_LPARAM(lparam)));
			break;
		}
		case WM_MOUSEMOVE:
		{
			OnMouseMove(MouseMoveControl(wparam), static_cast<float>(GET_X_LPARAM(lparam)), static_cast<float>(GET_Y_LPARAM(lparam)));
			break;
		}
		case WM_MOUSEHWHEEL:
		{
			OnMouseScrollWheel(GET_WHEEL_DELTA_WPARAM(wparam));
			break;
		}
		case WM_SIZE:
		{
			UINT Width = LOWORD(lparam);
			UINT Height = HIWORD(lparam);
			OnSize(Width, Height);
			break;
		}
		case WM_PAINT:
		{
			return OnPaint(hwnd);
		}
		default:
			return DefWindowProcW(hwnd, msg, wparam, lparam);
		}
		return 0;
	}

	[[nodiscard]] static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept
	{
		if (msg == WM_NCCREATE)
		{
			OutputDebugStringW(L"Received WM_NCCREATE\n");
			auto cs = reinterpret_cast<LPCREATESTRUCT>(lparam);
			Clz* pThis = reinterpret_cast<Clz*>(cs->lpCreateParams);
			pThis->m_hwnd = hwnd;
			return pThis->OnNcCreate(wparam, lparam);
		}
		else if (Clz* pThis = Handle(hwnd))
		{
			return pThis->WndProc(hwnd, msg, wparam, lparam);
		}
		return DefWindowProcW(hwnd, msg, wparam, lparam);
	}


public:
	HINSTANCE m_hInst{};
	HWND m_hwnd{};
};

template<typename T>
inline BaseWindow<T>::~BaseWindow()
{

}