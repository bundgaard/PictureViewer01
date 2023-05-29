
#include "Viewer.h"
#include <windowsx.h>

#include <sstream>

#include "BaseWindow.h"
#include "ZipFile.h"
#include "Converter.h"
#include "GraphicsManager.h"
#include <strsafe.h>
#include <algorithm>
#include <thread>

#include "ZipManager.h"
#include "resource.h"
#include "AnimatedImage.h"
#include "GraphicFactory.h"

#define CM_ZIP_LOADED WM_USER + 0
namespace
{
	constexpr wchar_t VIEWER_CLASSNAME[] = L"CPICTUREVIEWER01";

	float GetRatio(const float width, const float height)
	{
		return width / height;
	}

}


Viewer::Viewer() 
	: mGraphicFactory(GraphicFactory())
	, mGraphicManager(GraphicsManager(mGraphicFactory))
	, mZipManager(ZipManager())
	, mAnimImage(AnimatedImage(m_hwnd, mGraphicFactory))
	, mCurrentPage(0)
	
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
		mGraphicManager.Initialize(hwnd);
		hr = hwnd ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		LOG(L"Created Window\n");
	}
	DestroyIcon(hIcon);
	return hr;
}




inline LRESULT Viewer::OnPaint(const HWND hwnd) noexcept
{
	HRESULT hr = S_OK;
	PAINTSTRUCT ps{};

	hr = mGraphicManager.CreateDeviceResources(hwnd);
	if (SUCCEEDED(hr) && !(mGraphicManager.CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
	{
		mGraphicManager.RenderTarget()->BeginDraw();
		mGraphicManager.RenderTarget()->SetTransform(D2D1::IdentityMatrix());
		mGraphicManager.RenderTarget()->Clear();
		const auto [width, height] = mGraphicManager.RenderTarget()->GetSize();
		const auto color = mGraphicManager.Brush()->GetColor();


		if (mGraphicManager.Converter() && !mGraphicManager.Bitmap())
		{
			auto ptr = mGraphicManager.Bitmap();
			hr = mGraphicManager.RenderTarget()->CreateBitmapFromWicBitmap(mGraphicManager.Converter(), &ptr);
		}
		if (!mGraphicManager.Bitmap())
		{
			mGraphicManager.Brush()->SetColor(D2D1::ColorF(D2D1::ColorF::Crimson));
			mGraphicManager.RenderTarget()->FillRectangle(D2D1::RectF(0.f, 0.f, width, height), mGraphicManager.Brush());
			mGraphicManager.Brush()->SetColor(color);

			mGraphicManager.DrawTextCentered(L"[CTRL] + [o] - To open archive.", 50, D2D1::ColorF::White);
			mGraphicManager.DrawTextCentered(L"[PageUp] and [PageDown] to move back and forth between images in archive.", 100, D2D1::ColorF::White);
			mGraphicManager.DrawTextCentered(L"[ESC] to unload archive and return back to this menu.", 150, D2D1::ColorF::White);
		}
		if (mGraphicManager.Bitmap())
		{
			const auto [bitmapWidth, bitmapHeight] = mGraphicManager.Bitmap()->GetSize();

			constexpr float marginLeft = 50.0f;
			constexpr float marginRight = 50.0f;
			constexpr float marginTop = 50.0f;
			constexpr float marginBottom = 50.0f;

			const auto bitmapRatio = GetRatio(bitmapWidth, bitmapHeight);
			const auto windowRatio = GetRatio(width - (marginLeft + marginRight), height - (marginTop + marginBottom)); // 100.0f are the imaginary borders, will be moved somewhere

			float scaledWidth;
			float scaledHeight;


			if (bitmapRatio > windowRatio)
			{
				scaledWidth = width - (marginLeft + marginRight);
				scaledHeight = scaledWidth / bitmapRatio;
			}
			else
			{
				scaledHeight = height - (marginTop + marginBottom);
				scaledWidth = scaledHeight * bitmapRatio;
			}


			const auto clientRect = D2D1::RectF(
				(((width - marginLeft) - scaledWidth) / 2.0f),
				(((height - marginTop) - scaledHeight) / 2.0f),
				(((width + marginRight) + scaledWidth) / 2.0f),
				(((height + marginBottom) + scaledHeight) / 2.0f)
			);

			D2D1_MATRIX_3X2_F transform{};

			mGraphicManager.RenderTarget()->GetTransform(&transform);
			mGraphicManager.RenderTarget()->SetTransform(
				D2D1::Matrix3x2F::Scale(
					m_scaleFactor,
					m_scaleFactor,
					D2D1::Point2F(width / 2.0f, height / 2.0f)
				)
			);

			mGraphicManager.RenderTarget()->DrawBitmap(mGraphicManager.Bitmap(), clientRect);
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
		mZipManager.Previous();
		mCurrentPage = mZipManager.CurrentPage();
		UpdateTitle();
		if (const HRESULT hr = this->LoadImage(); SUCCEEDED(hr))
		{
			LOG(L"Loaded image\n");
		}

	}
	
	if (virtualKey == VK_NEXT) // Page Down
	{
		mZipManager.Next();
		mCurrentPage = mZipManager.CurrentPage();
		UpdateTitle();
		if (const HRESULT hr = LoadImage(); SUCCEEDED(hr))
		{
			LOG(L"Loaded image\n");
		}
	}

	if (virtualKey == VK_ESCAPE)
	{
		LOG(L"ESCAPE pressed\n");
		m_imageX = m_imageY = 0;

		mZipManager.Clear();
		mGraphicManager.ReleaseConverter();
		mGraphicManager.ReleaseDeviceResources();
		ResetTitle();
	}


	if (GetKeyState(VK_CONTROL) & 0x0800 && virtualKey == 0x4f) // [CTRL] + [o]
	{
		HRESULT hr = OpenArchive();
	}
	 if (virtualKey == 0x5a) // z
	{
		 
		 mAnimImage.Load(L"C:\\temp\\9o3d2q4dv02b1.gif");
	}
	if (virtualKey == 0x48)  // h
	{

	}
	if (virtualKey == 0x4c) // l
	{

	}

	InvalidateRect(m_hwnd, nullptr, true);
}


HRESULT Viewer::LoadFile(std::wstring const& path)
{
	return mGraphicManager.CreateBitmapFromFile(path);
}

HRESULT Viewer::LoadImage()
{
	HRESULT hr = S_OK;
	if (SUCCEEDED(hr))
	{
		hr = mZipManager.Size() > 0 ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{
		 std::unique_ptr<ZipFile>& item = mZipManager.Current();

		LOG(L"Create decoder from stream\n");
		hr = item->RecreateStream();
		if (SUCCEEDED(hr))
		{
			hr = mGraphicManager.CreateBitmapFromIStream(item->Stream);
		}
	}
	InvalidateRect(m_hwnd, nullptr, false);
	return hr;
}

HRESULT Viewer::OpenArchive()
{
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
		std::wstring Filename = ofn.lpstrFile;
		std::thread readArchiveThread(Viewer::ArchiveWorker, this, std::move(Filename));
		readArchiveThread.detach();
	}

	return hr;
}

void Viewer::OnSize(const UINT width, const UINT height) noexcept
{
	LOG(L"Received WM_SIZE %u,%u\n", width, height);
	mGraphicManager.Resize(static_cast<int>(width), static_cast<int>(height));
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

void Viewer::UpdateTitle()
{
	if (m_OriginalTitle.empty())
	{
		const int captionLength = GetWindowTextLengthW(m_hwnd);

		std::wstring caption;
		caption.resize(static_cast<size_t>(captionLength)+ 1);

		GetWindowTextW(m_hwnd, caption.data(), captionLength + 1);
		caption.resize(captionLength);
		m_OriginalTitle = caption;
	}

	std::wstringstream title;
	title << std::to_wstring(mCurrentPage + 1) << "/" << mZipManager.Size();
	std::wstring caption;
	caption += m_OriginalTitle;
	caption += L" ";
	caption += title.str();

	SetWindowText(m_hwnd, caption.c_str());

}

void Viewer::ResetTitle() const
{
	if (!m_OriginalTitle.empty())
	{
		SetWindowTextW(m_hwnd, m_OriginalTitle.c_str());
	}
}

void Viewer::ArchiveWorker(Viewer* viewer, std::wstring const& Filename)
{
	// This could be a problem, for now its detached and it works for my use case.
	// Could be considered bad practice, but its a background task that will be ready when its ready
	LOG(L"ArchiveWorker %s\n", Filename.c_str());
	viewer->mZipManager.ReadZip(Filename);
	viewer->m_imageX = viewer->m_imageY = 0;
	HRESULT hr = viewer->LoadImage();

	viewer->UpdateTitle();
	if (FAILED(hr))
	{
		LOG(L"Failed to load picture\n");
	}
}
