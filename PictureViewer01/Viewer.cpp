#include "Viewer.h"
#include <windowsx.h>
Viewer::Viewer() : m_bitmap(nullptr), m_wic_factory(nullptr), m_d2_factory(nullptr), m_brush(nullptr), m_renderTarget(nullptr)
{
}

Viewer::~Viewer()
{
	SafeRelease(m_brush);
	SafeRelease(m_bitmap);
	SafeRelease(m_wic_factory);
	SafeRelease(m_wic_converter);
	SafeRelease(m_d2_factory);
	SafeRelease(m_renderTarget);
}

HRESULT Viewer::Initialize(HINSTANCE hInst)
{
	WNDCLASSEX wc{};
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
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.cbWndExtra = sizeof(LONG_PTR);
		wc.cbClsExtra = 0;
		wc.lpszClassName = L"CPICTUREVIEWER01";
		wc.lpfnWndProc = (WNDPROC)Viewer::s_WndProc;
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.hIcon = wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.hInstance = hInst;

		m_hInst = hInst;

		hr = RegisterClassEx(&wc) ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		OutputDebugStringW(L"Created class\n");
		HWND hwnd = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
			L"CPICTUREVIEWER01",
			L"VIEWER",
			WS_VISIBLE | WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			640, 480,
			nullptr, nullptr,
			m_hInst,
			this);
		hr = hwnd ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{
		OutputDebugStringW(L"Created Window\n");
	}
	return hr;
}

HRESULT Viewer::CreateDeviceResources(HWND hwnd)
{
	HRESULT hr = S_OK;

	if (!m_renderTarget)
	{
		RECT rc{};
		hr = GetClientRect(hwnd, &rc) ? S_OK : E_FAIL;
		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Creating HWND render target\n");
			hr = m_d2_factory->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(640, 480)),
				&m_renderTarget);
			if (SUCCEEDED(hr))
			{
				hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_brush);
			}
		}
	}

	return hr;
}

inline LRESULT Viewer::OnPaint(HWND hwnd)
{
	HRESULT hr = S_OK;
	PAINTSTRUCT ps{};
	OutputDebugStringW(L"Received WM_PAINT\n");
	if (BeginPaint(hwnd, &ps))
	{
		OutputDebugStringW(L"Begun paint\n");
		hr = CreateDeviceResources(hwnd);

		if (SUCCEEDED(hr) && !(m_renderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
		{
			m_renderTarget->BeginDraw();
			m_renderTarget->SetTransform(D2D1::IdentityMatrix());
			m_renderTarget->Clear();
			auto color = m_brush->GetColor();
			m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::PaleVioletRed));
			m_renderTarget->FillRectangle(D2D1::RectF(0.f, 0.f, 640.f, 480.f), m_brush);
			m_brush->SetColor(color);
			m_renderTarget->DrawLine(D2D1::Point2F(0.0f, 0.0f), D2D1::Point2F(100.0f, 100.0f), m_brush);
			if (m_wic_converter && !m_bitmap)
			{
				m_renderTarget->CreateBitmapFromWicBitmap(m_wic_converter, &m_bitmap);
			}
			if (m_bitmap)
			{
				auto ClientSize = m_renderTarget->GetSize();
				auto BitmapSize = m_bitmap->GetSize();
				auto WidthRatio = std::abs(BitmapSize.width / ClientSize.width);
				auto HeightRatio = std::abs(BitmapSize.height / ClientSize.height);
				auto Ratio = std::min(HeightRatio, WidthRatio);
				wchar_t Buf[64] = { 0 };
				swprintf(Buf, 64, L"Ratios %f, %f %f, %f,%f\n", WidthRatio, HeightRatio, Ratio, m_imageX, m_imageY);
				OutputDebugStringW(Buf);

				auto ClientRect = D2D1::RectF(m_imageX, m_imageY, m_imageX + BitmapSize.width / Ratio, m_imageY + BitmapSize.height / Ratio);

				m_renderTarget->DrawBitmap(m_bitmap, ClientRect);
			}
			//	m_renderTarget->DrawBitmap(m_bitmap);
			hr = m_renderTarget->EndDraw();
			if (hr == D2DERR_RECREATE_TARGET)
			{
				SafeRelease(m_brush);
				SafeRelease(m_bitmap);
				SafeRelease(m_renderTarget);

				hr = InvalidateRect(hwnd, nullptr, true) ? S_OK : E_FAIL;
			}
		}
		EndPaint(hwnd, &ps);
	}
	return SUCCEEDED(hr) ? S_OK : E_FAIL;
}

inline LRESULT Viewer::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
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

		if (m_renderTarget != nullptr)
		{
			if (FAILED(m_renderTarget->Resize(D2D1::SizeU(Width, Height))))
			{
				SafeRelease(m_renderTarget);
				SafeRelease(m_bitmap);
				SafeRelease(m_brush);
			}
		}
	}
	break;

	case WM_LBUTTONDOWN:
	{
		SetCapture(hwnd);
		m_lastMouseX = static_cast<float>(GET_X_LPARAM(lparam));
		m_lastMouseY = static_cast<float>(GET_Y_LPARAM(lparam));
	}
	return 0;
	case WM_LBUTTONUP:
	{
		ReleaseCapture();
	}
	return 0;
	case WM_MOUSEMOVE:
	{
		OnMouseMove(MouseMoveControl(wparam), GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
	}
	return 0;
	case WM_KEYDOWN:
	{
		if (wparam == VK_SPACE)
		{
			HRESULT hr = this->LoadFile(PICTURE);
			if (SUCCEEDED(hr))
			{
				OutputDebugStringW(L"Loaded file\n");

			}
		}

		if (wparam == VK_ESCAPE)
		{
			m_imageX = 0;
			m_imageY = 0;

		}
		InvalidateRect(hwnd, nullptr, true);
	}
	return 0;
	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
	return 0;
	default:
		return DefWindowProcW(hwnd, msg, wparam, lparam);
	}
	return 0;
}

HRESULT Viewer::LoadFile(std::wstring const& Path)
{
	HRESULT hr = S_OK;
	IWICBitmapDecoder* decoder = nullptr;
	if (SUCCEEDED(hr))
	{
		hr = m_wic_factory->CreateDecoderFromFilename(Path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
	}

	IWICBitmapFrameDecode* frame = nullptr;
	if (SUCCEEDED(hr))
	{
		hr = decoder->GetFrame(0, &frame);
	}

	if (SUCCEEDED(hr))
	{
		SafeRelease(m_wic_converter);
		hr = m_wic_factory->CreateFormatConverter(&m_wic_converter);
	}

	if (SUCCEEDED(hr))
	{
		hr = m_wic_converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
	}

	if (SUCCEEDED(hr))
	{
		hr = CreateDeviceResources(m_hwnd);
	}

	if (SUCCEEDED(hr))
	{
		SafeRelease(m_bitmap);
		hr = m_renderTarget->CreateBitmapFromWicBitmap(m_wic_converter, &m_bitmap);
	}

	SafeRelease(decoder);
	SafeRelease(frame);

	return hr;
}

LRESULT Viewer::OnNcCreate(WPARAM wparam, LPARAM lparam)
{
	SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	return DefWindowProcW(m_hwnd, WM_NCCREATE, wparam, lparam);

}

void Viewer::OnMouseMove(MouseMoveControl ctrl, int x, int y)
{
	switch (ctrl)
	{
	case MouseMoveControl::LeftButton:
	{
		auto pt = D2D1::Point2F(static_cast<float>(x), static_cast<float>(y));
		float dx = static_cast<float>(pt.x - m_lastMouseX);
		float dy = static_cast<float>(pt.y - m_lastMouseY);
		m_lastMouseX = pt.x;
		m_lastMouseY = pt.y;

		m_imageX += dx;
		m_imageY += dy;
		InvalidateRect(m_hwnd, nullptr, false);
	}
	break;
	}
}
