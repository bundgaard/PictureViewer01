
#include "Viewer.h"
#include <windowsx.h>



#include <sstream>
#include <array>

#include "BaseWindow.h"
#include "SafeRelease.h"
#include "ZipFile.h"
#include "Converter.h"
#include "GraphicsManager.h"
#include <strsafe.h>
#include <algorithm>

#include "ZipManager.h"

namespace
{

	constexpr wchar_t VIEWER_CLASSNAME[] = L"CPICTUREVIEWER01";
	inline float GetRatio(float width, float height)
	{
		return width / height;
	}

}


Viewer::Viewer(GraphicsManager& graphicManager, ZipManager& zipManager) : mGraphicManager(graphicManager), m_ZipManager(zipManager)
{
}

Viewer::~Viewer()
{
	LOG(L"Viewer DTOR\n");
}

HRESULT Viewer::Initialize(HINSTANCE hInst)
{
	WNDCLASSEX wc{};
	HRESULT hr = S_OK;

	if (SUCCEEDED(hr))
	{
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.cbWndExtra = sizeof(LONG_PTR);
		wc.cbClsExtra = 0;
		wc.lpszClassName = VIEWER_CLASSNAME;
		wc.lpfnWndProc = (WNDPROC)Viewer::s_WndProc;
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.hIcon = wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.hInstance = hInst;

		m_hInst = hInst;

		hr = RegisterClassEx(&wc) ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		LOG(L"Created class\n");
		HWND hwnd = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
			L"CPICTUREVIEWER01",
			L"VIEWER",
			WS_VISIBLE | WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			640, 480,
			nullptr, nullptr,
			m_hInst,
			this);
		mGraphicManager.Initialize(hwnd);
		hr = hwnd ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		LOG(L"Created Window\n");
	}
	return hr;
}




inline LRESULT Viewer::OnPaint(HWND hwnd) noexcept
{
	HRESULT hr = S_OK;
	PAINTSTRUCT ps{};

	hr = mGraphicManager.CreateDeviceResources(hwnd);
	if (SUCCEEDED(hr) && !(mGraphicManager.CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
	{
		mGraphicManager.RenderTarget()->BeginDraw();
		mGraphicManager.RenderTarget()->SetTransform(D2D1::IdentityMatrix());
		mGraphicManager.RenderTarget()->Clear();
		auto ClientSize = mGraphicManager.RenderTarget()->GetSize();
		auto color = mGraphicManager.Brush()->GetColor();


		if (mGraphicManager.Converter() && !mGraphicManager.Bitmap())
		{
			auto ptr = mGraphicManager.Bitmap();
			mGraphicManager.RenderTarget()->CreateBitmapFromWicBitmap(mGraphicManager.Converter(), &ptr);
		}
		if (!mGraphicManager.Bitmap())
		{
			mGraphicManager.Brush()->SetColor(D2D1::ColorF(D2D1::ColorF::Crimson));
			mGraphicManager.RenderTarget()->FillRectangle(D2D1::RectF(0.f, 0.f, ClientSize.width, ClientSize.height), mGraphicManager.Brush());
			mGraphicManager.Brush()->SetColor(color);

			mGraphicManager.DrawTextCentered(L"[CTRL] + [o] - To open archive.", 50, D2D1::ColorF::White);
			mGraphicManager.DrawTextCentered(L"[PageUp] and [PageDown] to move back and forth between images in archive.", 100, D2D1::ColorF::White);
			mGraphicManager.DrawTextCentered(L"[ESC] to unload archive and return back to this menu.", 150, D2D1::ColorF::White);
		}
		if (mGraphicManager.Bitmap())
		{
			auto BitmapSize = mGraphicManager.Bitmap()->GetSize();

			const float marginLeft = 50.0f;
			const float marginRight = 50.0f;
			const float marginTop = 50.0f;
			const float marginBottom = 50.0f;

			auto BitmapRatio = GetRatio(BitmapSize.width, BitmapSize.height);
			auto WindowRatio = GetRatio(ClientSize.width - (marginLeft + marginRight), ClientSize.height - (marginTop + marginBottom)); // 100.0f are the imaginary borders, will be moved somewhere

			float scaledWidth{};
			float scaledHeight{};


			if (BitmapRatio > WindowRatio)
			{
				scaledWidth = ClientSize.width - (marginLeft + marginRight);
				scaledHeight = scaledWidth / BitmapRatio;
			}
			else
			{
				scaledHeight = ClientSize.height - (marginTop + marginBottom);
				scaledWidth = scaledHeight * BitmapRatio;
			}


			auto ClientRect = D2D1::RectF(
				(((ClientSize.width - marginLeft) - scaledWidth) / 2.0f),
				(((ClientSize.height - marginTop) - scaledHeight) / 2.0f),
				(((ClientSize.width + marginRight) + scaledWidth) / 2.0f),
				(((ClientSize.height + marginBottom) + scaledHeight) / 2.0f)
			);

			D2D1_MATRIX_3X2_F transform{};

			mGraphicManager.RenderTarget()->GetTransform(&transform);
			mGraphicManager.RenderTarget()->SetTransform(
				D2D1::Matrix3x2F::Scale(
					m_scaleFactor,
					m_scaleFactor,
					D2D1::Point2F(
						((ClientSize.width - 50.0f) - scaledWidth) / 2.0f,
						((ClientSize.height - 50.0f) - scaledHeight) / 2.0f)
				)
			);

			mGraphicManager.RenderTarget()->DrawBitmap(mGraphicManager.Bitmap(), ClientRect);
			/*mGraphicManager.RenderTarget()->SetTransform(transform);*/
		}
		hr = mGraphicManager.RenderTarget()->EndDraw();
		if (hr == D2DERR_RECREATE_TARGET)
		{
			mGraphicManager.ReleaseDeviceResources();
			hr = InvalidateRect(hwnd, nullptr, true) ? S_OK : E_FAIL;
		}
	}
	return SUCCEEDED(hr) ? S_OK : E_FAIL;
}

void Viewer::OnKeyDown(UINT32 VirtualKey) noexcept
{
#ifdef _DEBUG

	std::wstringstream Out;
	Out << std::hex << VirtualKey << L"\n";
	LOG(Out.str().c_str());
#endif
	if (VirtualKey == VK_SPACE)
	{
		m_imageX = m_imageY = 0.0f;
	}

	if (VirtualKey == VK_PRIOR) // PageUp
	{
		m_ZipManager.Previous();
		HRESULT hr = this->LoadImage(0);
		if (SUCCEEDED(hr))
		{
			LOG(L"Loaded image\n");
		}

	}

	if (VirtualKey == VK_NEXT) // Page Down
	{

		m_ZipManager.Next();
		HRESULT hr = LoadImage(0);
		if (SUCCEEDED(hr))
		{
			LOG(L"Loaded image\n");
		}
	}

	if (VirtualKey == VK_ESCAPE)
	{
		LOG(L"ESCAPE pressed\n");
		m_imageX = m_imageY = 0;
		m_ZipManager.Clear();
		mGraphicManager.ReleaseConverter();
		mGraphicManager.ReleaseDeviceResources();
	}


	if (GetKeyState(VK_CONTROL) & 0x0800 && VirtualKey == 0x4f) // o
	{
		HRESULT hr = OpenArchive();
		if (SUCCEEDED(hr))
		{
			m_imageX = m_imageY = 0;
			this->LoadImage(+1);
		}
	}
	InvalidateRect(m_hwnd, nullptr, true);
}


HRESULT Viewer::LoadFile(std::wstring const& Path)
{
	HRESULT hr = S_OK;
	mGraphicManager.CreateBitmapFromFile(Path);
	return hr;
}

HRESULT Viewer::LoadImage(int delta)
{
	HRESULT hr = S_OK;
	if (SUCCEEDED(hr))
	{
		hr = m_ZipManager.Size() > 0 ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{	
		std::unique_ptr<ZipFile>& item = m_ZipManager.Current();

		LOG(L"Create decoder from stream\n");
		hr = item->RecreateStream();
		if (SUCCEEDED(hr))
		{
			hr = mGraphicManager.CreateBitmapFromIStream(item->Stream);
		}
	}

	return hr;
}

HRESULT Viewer::OpenArchive()
{

	// openFile dialog with only zip archive.
	OPENFILENAME ofn{};
	wchar_t szFile[MAX_PATH] = { 0 };

	ofn.hwndOwner = m_hwnd;
	ofn.lStructSize = sizeof(ofn);
	ofn.hInstance = (HINSTANCE)GetModuleHandleW(0);
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = L'\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"ZIP Archive\0*.zip";
	ofn.lpstrTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	HRESULT hr = GetOpenFileNameW(&ofn) ? S_OK : E_FAIL;
	if (SUCCEEDED(hr))
	{
		LOG(L"Received %s\n", ofn.lpstrFile);


		m_scaleFactor = 1.0f;
		m_ZipManager.Clear();
		m_ZipManager.ReadZip(ofn.lpstrFile);
	}

	return hr;
}

void Viewer::OnSize(UINT Width, UINT Height) noexcept
{
	LOG(L"Received WM_SIZE %u,%u\n", Width, Height);
	mGraphicManager.Resize(Width, Height);
}

void Viewer::OnMouseMove(MouseMoveControl ctrl, float x, float y) noexcept
{
	switch (ctrl)
	{
	case MouseMoveControl::LeftButton:
	{
		auto pt = D2D1::Point2F(x, y);

		float dx = pt.x - m_lastMouseX;
		float dy = pt.y - m_lastMouseY;

		m_lastMouseX = pt.x;
		m_lastMouseY = pt.y;

		m_imageX += dx;
		m_imageY += dy;
		InvalidateRect(m_hwnd, nullptr, false);
	}
	break;
	}
}

void Viewer::OnLButtonDown(float x, float y) noexcept
{

	SetCapture(m_hwnd);
	m_lastMouseX = x;
	m_lastMouseY = y;
}

void Viewer::OnLButtonUp(float x, float y) noexcept
{
	ReleaseCapture();
}

void Viewer::OnMouseScrollWheel(short delta) noexcept
{
	if (delta > 0)
	{
		m_scaleFactor *= 1.1f;
	}
	else if (delta < 0)
	{
		m_scaleFactor /= 1.1f;
	}

	LOG(L"Zoom %f\n", (1.0f - m_scaleFactor) * 100.f);

	InvalidateRect(m_hwnd, nullptr, false);
}

void Viewer::OnChar(wchar_t KeyCode, short RepeatCount) noexcept
{
}

void Viewer::Start()
{
	MSG msg{};
	while (GetMessage(&msg, nullptr, 0, 0) != 0)
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}
