
#include "Viewer.h"
#include <windowsx.h>

#include <sstream>

#include "BaseWindow.h"
#include "ZipFile.h"
#include "Converter.h"
#include "GraphicsManager.h"
#include <strsafe.h>
#include <algorithm>

#include "ZipManager.h"
#include "resource.h"

namespace
{

	constexpr wchar_t VIEWER_CLASSNAME[] = L"CPICTUREVIEWER01";

	float GetRatio(const float width, const float height)
	{
		return width / height;
	}

}


Viewer::Viewer() : mGraphicManager(std::make_unique<GraphicsManager>()), m_ZipManager(std::make_unique<ZipManager>())
{
}

Viewer::~Viewer()
{
	LOG(L"Viewer DTOR\n");
}

HRESULT Viewer::Initialize(const HINSTANCE hInst)
{
	WNDCLASSEX wc{};
	HRESULT hr = S_OK;
	const auto hIcon = static_cast<HICON>(::LoadImageW(hInst, MAKEINTRESOURCEW(IDB_BITMAP1), IMAGE_ICON, 64, 64, LR_DEFAULTCOLOR));
	if (SUCCEEDED(hr))
	{
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.cbWndExtra = sizeof(LONG_PTR);
		wc.cbClsExtra = 0;
		wc.lpszClassName = VIEWER_CLASSNAME;
		wc.lpfnWndProc = static_cast<WNDPROC>(s_WndProc);
		wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
		wc.hIcon = hIcon;
		wc.hIconSm = hIcon;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.hInstance = hInst;

		m_hInst = hInst;

		hr = RegisterClassEx(&wc) ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		LOG(L"Created class\n");
		const HWND hwnd = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
			L"CPICTUREVIEWER01",
			L"VIEWER",
			WS_VISIBLE | WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			640, 480,
			nullptr, nullptr,
			m_hInst,
			this);
		mGraphicManager->Initialize(hwnd);
		hr = hwnd ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		LOG(L"Created Window\n");
	}
	return hr;
}




inline LRESULT Viewer::OnPaint(const HWND hwnd) noexcept
{
	HRESULT hr = S_OK;
	PAINTSTRUCT ps{};

	hr = mGraphicManager->CreateDeviceResources(hwnd);
	if (SUCCEEDED(hr) && !(mGraphicManager->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
	{
		mGraphicManager->RenderTarget()->BeginDraw();
		mGraphicManager->RenderTarget()->SetTransform(D2D1::IdentityMatrix());
		mGraphicManager->RenderTarget()->Clear();
		const auto clientSize = mGraphicManager->RenderTarget()->GetSize();
		const auto color = mGraphicManager->Brush()->GetColor();


		if (mGraphicManager->Converter() && !mGraphicManager->Bitmap())
		{
			auto ptr = mGraphicManager->Bitmap();
			hr = mGraphicManager->RenderTarget()->CreateBitmapFromWicBitmap(mGraphicManager->Converter(), &ptr);
		}
		if (!mGraphicManager->Bitmap())
		{
			mGraphicManager->Brush()->SetColor(D2D1::ColorF(D2D1::ColorF::Crimson));
			mGraphicManager->RenderTarget()->FillRectangle(D2D1::RectF(0.f, 0.f, clientSize.width, clientSize.height), mGraphicManager->Brush());
			mGraphicManager->Brush()->SetColor(color);

			mGraphicManager->DrawTextCentered(L"[CTRL] + [o] - To open archive.", 50, D2D1::ColorF::White);
			mGraphicManager->DrawTextCentered(L"[PageUp] and [PageDown] to move back and forth between images in archive.", 100, D2D1::ColorF::White);
			mGraphicManager->DrawTextCentered(L"[ESC] to unload archive and return back to this menu.", 150, D2D1::ColorF::White);
		}
		if (mGraphicManager->Bitmap())
		{
			const auto bitmapSize = mGraphicManager->Bitmap()->GetSize();

			constexpr float marginLeft = 50.0f;
			constexpr float marginRight = 50.0f;
			constexpr float marginTop = 50.0f;
			constexpr float marginBottom = 50.0f;

			const auto bitmapRatio = GetRatio(bitmapSize.width, bitmapSize.height);
			const auto windowRatio = GetRatio(clientSize.width - (marginLeft + marginRight), clientSize.height - (marginTop + marginBottom)); // 100.0f are the imaginary borders, will be moved somewhere

			float scaledWidth{};
			float scaledHeight{};


			if (bitmapRatio > windowRatio)
			{
				scaledWidth = clientSize.width - (marginLeft + marginRight);
				scaledHeight = scaledWidth / bitmapRatio;
			}
			else
			{
				scaledHeight = clientSize.height - (marginTop + marginBottom);
				scaledWidth = scaledHeight * bitmapRatio;
			}


			const auto clientRect = D2D1::RectF(
				(((clientSize.width - marginLeft) - scaledWidth) / 2.0f),
				(((clientSize.height - marginTop) - scaledHeight) / 2.0f),
				(((clientSize.width + marginRight) + scaledWidth) / 2.0f),
				(((clientSize.height + marginBottom) + scaledHeight) / 2.0f)
			);

			D2D1_MATRIX_3X2_F transform{};

			mGraphicManager->RenderTarget()->GetTransform(&transform);
			mGraphicManager->RenderTarget()->SetTransform(
				D2D1::Matrix3x2F::Scale(
					m_scaleFactor,
					m_scaleFactor,
					D2D1::Point2F(
						((clientSize.width - 50.0f) - scaledWidth) / 2.0f,
						((clientSize.height - 50.0f) - scaledHeight) / 2.0f)
				)
			);

			mGraphicManager->RenderTarget()->DrawBitmap(mGraphicManager->Bitmap(), clientRect);
			/*mGraphicManager.RenderTarget()->SetTransform(transform);*/
		}
		hr = mGraphicManager->RenderTarget()->EndDraw();
		if (hr == D2DERR_RECREATE_TARGET)
		{
			mGraphicManager->ReleaseDeviceResources();
			hr = InvalidateRect(hwnd, nullptr, true) ? S_OK : E_FAIL;
		}
	}
	return SUCCEEDED(hr) ? S_OK : E_FAIL;
}

void Viewer::OnKeyDown(const UINT32 virtualKey) noexcept
{
#ifdef _DEBUG

	std::wstringstream out;
	out << std::hex << virtualKey << L"\n";
	LOG(out.str().c_str());
#endif
	if (virtualKey == VK_SPACE)
	{
		m_imageX = m_imageY = 0.0f;
	}

	if (virtualKey == VK_PRIOR) // PageUp
	{
		m_ZipManager->Previous();
		const HRESULT hr = this->LoadImage(0);
		if (SUCCEEDED(hr))
		{
			LOG(L"Loaded image\n");
		}

	}

	if (virtualKey == VK_NEXT) // Page Down
	{

		m_ZipManager->Next();
		const HRESULT hr = LoadImage(0);
		if (SUCCEEDED(hr))
		{
			LOG(L"Loaded image\n");
		}
	}

	if (virtualKey == VK_ESCAPE)
	{
		LOG(L"ESCAPE pressed\n");
		m_imageX = m_imageY = 0;
		m_ZipManager->Clear();
		mGraphicManager->ReleaseConverter();
		mGraphicManager->ReleaseDeviceResources();
		ResetTitle();
		
	}


	if (GetKeyState(VK_CONTROL) & 0x0800 && virtualKey == 0x4f) // o
	{
		HRESULT hr = OpenArchive();
		if (SUCCEEDED(hr))
		{
			m_imageX = m_imageY = 0;
			hr = this->LoadImage(+1);
		}

		if (FAILED(hr))
		{
			LOG(L"Failed to load picture\n");
		}
	}
	InvalidateRect(m_hwnd, nullptr, true);
}


HRESULT Viewer::LoadFile(std::wstring const& path) const
{
	return mGraphicManager->CreateBitmapFromFile(path);
}

HRESULT Viewer::LoadImage(int delta) const
{
	HRESULT hr = S_OK;
	if (SUCCEEDED(hr))
	{
		hr = m_ZipManager->Size() > 0 ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{
		const std::unique_ptr<ZipFile>& item = m_ZipManager->Current();

		LOG(L"Create decoder from stream\n");
		hr = item->RecreateStream();
		if (SUCCEEDED(hr))
		{
			hr = mGraphicManager->CreateBitmapFromIStream(item->Stream);
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
	const HRESULT hr = GetOpenFileNameW(&ofn) ? S_OK : E_FAIL;
	if (SUCCEEDED(hr))
	{
		LOG(L"Received %s\n", ofn.lpstrFile);


		m_scaleFactor = 1.0f;
		m_ZipManager->Clear();
		m_ZipManager->ReadZip(ofn.lpstrFile);
		AppendTitle(L"");
		
	}

	return hr;
}

void Viewer::OnSize(const UINT width, const UINT height) noexcept
{
	LOG(L"Received WM_SIZE %u,%u\n", width, height);
	mGraphicManager->Resize(width, height);
}

void Viewer::OnMouseMove(const MouseMoveControl ctrl, const float x, const float y) noexcept
{
	switch (ctrl)
	{
	case MouseMoveControl::LeftButton:
	{
		const auto [_x, _y] = D2D1::Point2F(x, y);

		const float dx = _x - m_lastMouseX;
		const float dy = _y - m_lastMouseY;

		m_lastMouseX = _x;
		m_lastMouseY = _y;

		m_imageX += dx;
		m_imageY += dy;
		InvalidateRect(m_hwnd, nullptr, false);
	}
	break;
	case MouseMoveControl::Ctrl: break;
	case MouseMoveControl::MiddleButton: break;
	case MouseMoveControl::RightButton: break;
	case MouseMoveControl::Shift: break;
	case MouseMoveControl::XButton1: break;
	case MouseMoveControl::XButton2: break;
	}
}

void Viewer::OnLButtonDown(const float x, const float y) noexcept
{

	SetCapture(m_hwnd);
	m_lastMouseX = x;
	m_lastMouseY = y;
}

void Viewer::OnLButtonUp(float x, float y) noexcept
{
	ReleaseCapture();
}

void Viewer::OnMouseScrollWheel(const short delta) noexcept
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

void Viewer::OnChar(wchar_t keyCode, short repeatCount) noexcept
{
}

void Viewer::Start() noexcept
{
	MSG msg{};
	while (GetMessage(&msg, nullptr, 0, 0) != 0)
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

void Viewer::AppendTitle(std::wstring const& aTitle)
{
	int CaptionLength = GetWindowTextLengthW(m_hwnd);
	OutputDebugStringW(std::to_wstring(CaptionLength).c_str());
	std::wstring Caption;
	Caption.resize(CaptionLength + 1);
	GetWindowTextW(m_hwnd, Caption.data(), CaptionLength + 1);
	m_OriginalTitle = Caption;
	Caption.resize(CaptionLength); // reduce \0
	Caption += L" ";
	Caption += std::to_wstring(m_ZipManager.Size());
	Caption += L" files loaded";
	SetWindowTextW(m_hwnd, Caption.c_str());
	OutputDebugStringW(Caption.c_str());
}

void Viewer::ResetTitle()
{
	if (m_OriginalTitle.size() != 0)
	{
		SetWindowTextW(m_hwnd, m_OriginalTitle.c_str());
	}
}
