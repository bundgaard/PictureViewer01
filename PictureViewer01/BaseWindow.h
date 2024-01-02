#pragma once
#define NOMINMAX
#define NODRAWTEXT
#define NOHELP
#define NOMCX
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
	static Clz* Handle(const HWND hwnd) noexcept
	{
		return reinterpret_cast<Clz*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
	}

	virtual ~BaseWindow() = 0;
	virtual HRESULT Initialize(HINSTANCE hInst) = 0;

	[[nodiscard]] virtual LRESULT OnPaint(HWND hwnd) noexcept = 0;


	virtual void OnSize(UINT width, UINT height) noexcept = 0;
	virtual void OnLButtonDown(float x, float y) noexcept = 0;
	virtual void OnLButtonUp(float x, float y) noexcept = 0;
	virtual void OnMouseMove(MouseMoveControl ctrl, float x, float y) noexcept = 0;
	virtual void OnMouseScrollWheel(short delta) noexcept = 0;
	virtual void OnKeyDown(UINT32 virtualKey) noexcept = 0;
	virtual void OnChar(wchar_t keyCode, short repeatCount) noexcept = 0;
	virtual void OnTimer() noexcept = 0;
	virtual void OnDpiChanged(int dpiX, int dpiY, RECT rect) noexcept = 0;


	[[nodiscard]] virtual LRESULT OnNcCreate(const WPARAM wparam, const LPARAM lparam) noexcept
	{
		OutputDebugStringW(L"In OnNcCreate\n");
		SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		return DefWindowProcW(m_hwnd, WM_NCCREATE, wparam, lparam);
	}
	[[nodiscard]] virtual LRESULT WndProc(const HWND hwnd, const UINT msg, WPARAM wparam, LPARAM lparam) noexcept
	{
		switch (msg)
		{
		case WM_DPICHANGED:
		{
			/*
			wParam
			The HIWORD of the wParam contains the Y-axis value of the new dpi of the window.
			The LOWORD of the wParam contains the X-axis value of the new DPI of the window.
			For example, 96, 120, 144, or 192.
			The values of the X-axis and the Y-axis are identical for Windows apps.
			lParam
			A pointer to a RECT structure that provides a suggested size and position
			of the current window scaled for the new DPI.
			The expectation is that apps will reposition and resize windows based on the
			suggestions provided by lParam when handling this message.
			*/
			auto* SuggestedRect = reinterpret_cast<RECT*>(lparam);
			auto dpiX = HIWORD(wparam);
			auto dpiY = LOWORD(wparam);
			OnDpiChanged(dpiX, dpiY, *SuggestedRect);
			return 0;
		}
		case WM_MOUSEWHEEL:
		{
			OnMouseScrollWheel(GET_WHEEL_DELTA_WPARAM(wparam));
			return 0;
		}
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
			OnMouseMove(
				static_cast<MouseMoveControl>(wparam),
				static_cast<float>(GET_X_LPARAM(lparam)),
				static_cast<float>(GET_Y_LPARAM(lparam))
			);
			break;
		}

		case WM_SIZE:
		{
			const UINT width = LOWORD(lparam);
			const UINT height = HIWORD(lparam);
			OnSize(width, height);
			break;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);
			const auto result = OnPaint(hwnd);
			EndPaint(hwnd, &ps);
			return result;
		}
		case WM_CHAR:
		{
			OnChar(
				static_cast<TCHAR>(wparam),
				static_cast<short>(LOWORD(lparam))
			);
			break;
		}
		case WM_TIMER:
		{
			OnTimer();
			break;
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
			const auto cs = reinterpret_cast<LPCREATESTRUCT>(lparam);
			Clz* pThis = static_cast<Clz*>(cs->lpCreateParams);
			pThis->m_hwnd = hwnd;
			return pThis->OnNcCreate(wparam, lparam);
		}
		if (Clz* pThis = Handle(hwnd))
		{
			return pThis->WndProc(hwnd, msg, wparam, lparam);
		}
		return DefWindowProcW(hwnd, msg, wparam, lparam);
	}

	HINSTANCE m_hInst{};
	HWND m_hwnd{};
};

template<typename T>
BaseWindow<T>::~BaseWindow() = default;
//template<typename T>
//HRESULT  BaseWindow<T>::Initialize(HINSTANCE hInst) noexcept {}
//
//template<typename T>
//LRESULT BaseWindow<T>::OnPaint(HWND hwnd)noexcept {}
//
//template<typename T>
//void BaseWindow<T>::OnSize(UINT Width, UINT Height)noexcept {}
//
//template<typename T>
//void BaseWindow<T>::OnLButtonDown(float x, float y)noexcept {}
//
//template<typename T>
//void BaseWindow<T>::OnLButtonUp(float x, float y)noexcept {}
//
//template<typename T>
//void BaseWindow<T>::OnMouseMove(MouseMoveControl ctrl, float x, float y)noexcept {}
//
//template<typename T>
//void BaseWindow<T>::OnMouseScrollWheel(short delta)noexcept {}
//
//template<typename T>
//inline void BaseWindow<T>::OnKeyDown(UINT32 VirtualKey) noexcept {}