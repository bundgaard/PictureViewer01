
#include <algorithm>
#include <thread>
#include <sstream>

#include "Viewer.h"
#include "ZipManager.h"
#include "resource.h"
#include "AnimatedImage.h"
#include "GraphicFactory.h"
#include "ZipFile.h"
#include "Converter.h"
#include "GraphicsManager.h"

#include <windowsx.h>
#include <strsafe.h>
#include <array>
#include <atlbase.h>
#include <shobjidl.h>
#include <shlwapi.h>


#define CM_ZIP_LOADED WM_USER + 0

namespace
{
	constexpr wchar_t VIEWER_CLASSNAME[] = L"CPICTUREVIEWER01";
	constexpr wchar_t NEWLINE[] = L"\r\n";
}


Viewer::Viewer()
	: mGraphicFactory(GraphicFactory())
	, mGraphicManager(GraphicsManager(mGraphicFactory))
	, mZipManager(ZipManager())
	, mAnimImage(AnimatedImage(mGraphicFactory))
	, mBossMode(BossMode(mGraphicFactory))
	, mCurrentPage(0)
	, mDpi(96)
{


}

Viewer::~Viewer()
{
	LOG(L"Viewer DTOR\n");
	KillTimer(m_hwnd, 0);
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
		const HWND hwnd = CreateWindowExW(
			WS_EX_OVERLAPPEDWINDOW,
			CLASSNAME,
			TITLE,
			WS_VISIBLE | WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			640, 480,
			nullptr, nullptr,
			m_hInst,
			this
		);
		mDpi = GetDpiForWindow(hwnd);
		SetWindowPos(
			hwnd,
			nullptr,
			0, 0,
			static_cast<int>(640.0f * static_cast<float>(mDpi) / 96.0f),
			static_cast<int>(480.0f * static_cast<float>(mDpi) / 96.0f),
			SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER
		);
		mGraphicManager.Initialize(hwnd, mDpi);
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

	// MOVE THE ID2D1HWND RENDER TO VIEWER AND SEPARATE IT FORM mGraphicManager

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
		CComPtr<ID2D1RadialGradientBrush> radialBrush = nullptr;
		CComPtr<ID2D1GradientStopCollection> collection = nullptr;
		std::array<D2D1_GRADIENT_STOP, 3> const stops = {
			{

				D2D1::GradientStop(0.0f, D2D1::ColorF(D2D1::ColorF::DarkGray)),
				D2D1::GradientStop(0.50f, D2D1::ColorF(D2D1::ColorF::Crimson)),
				D2D1::GradientStop(1.0f, D2D1::ColorF(D2D1::ColorF::Black)),
			}
		};


		hr = mGraphicManager.RenderTarget()->CreateGradientStopCollection(
			stops.data(), 
			static_cast<int>(stops.size()), 
			&collection);

		if (SUCCEEDED(hr))
		{

			hr = mGraphicManager.RenderTarget()->CreateRadialGradientBrush(
				D2D1::RadialGradientBrushProperties(
					D2D1::Point2F(width / 2.0f, height / 2.0f),
					D2D1::Point2F(),
					width / 2.0f, height / 2.0f
				),
				collection,
				&radialBrush
			);
		}

		mGraphicManager.Brush()->SetColor(D2D1::ColorF(D2D1::ColorF::Crimson));
		if (SUCCEEDED(hr))
		{
			mGraphicManager.RenderTarget()->FillRectangle(D2D1::RectF(0.f, 0.f, width, height), radialBrush);
		}

		mGraphicManager.Brush()->SetColor(color);
		auto* const information = L"[CTRL] + [O] - To open archive."
			L"\r\n"
			L"[CTRL] + [F] - To open folder"
			L"\r\n"
			L"[PageUp] and [PageDown] - to move back and forth between images in archive."
			L"\r\n"
			L"[ESC] - to unload archive and return back to this menu."
			;
		mGraphicManager.DrawTextCentered(information, 50, D2D1::ColorF::White);
		if (mGraphicManager.Bitmap())
		{

			const auto [bitmapWidth, bitmapHeight] = mGraphicManager.Bitmap()->GetSize();

			constexpr float marginLeft = 50.0f;
			constexpr float marginRight = 50.0f;
			constexpr float marginTop = 50.0f;
			constexpr float marginBottom = 50.0f;

			const auto bitmapRatio = mGraphicFactory.GetRatio(bitmapWidth, bitmapHeight);
			const auto windowRatio = mGraphicFactory.GetRatio(width - (marginLeft + marginRight), height - (marginTop + marginBottom)); // 100.0f are the imaginary borders, will be moved somewhere

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

			const auto bitmapRect = D2D1::RectF(
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
			mGraphicManager.Brush()->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
			mGraphicManager.RenderTarget()->FillRectangle(D2D1::RectF(0, 0, width, height), mGraphicManager.Brush());
			mGraphicManager.Brush()->SetColor(color);
			mGraphicManager.RenderTarget()->DrawBitmap(mGraphicManager.Bitmap(), bitmapRect);
			mGraphicManager.RenderTarget()->SetTransform(transform);
		}

		mAnimImage.Render(mGraphicManager.RenderTarget());
		mBossMode.Render(mGraphicManager.RenderTarget());

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
	// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
#if defined(DEBUG) ||defined(_DEBUG)

	std::wstringstream out;
	out << std::hex << virtualKey << L"\n";
	LOG(out.str().c_str());
#endif
	if (!mBossMode.IsActive())
	{
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
			mAnimImage.SetLoaded(false); // BUG in these four lines

			mZipManager.Clear();
			mGraphicManager.ReleaseConverter();
			mGraphicManager.ReleaseDeviceResources();
			ResetTitle();
		}


		if (GetKeyState(VK_CONTROL) & 0x0800 && virtualKey == 0x4f) // [CTRL] + [o]
		{
			HRESULT hr = OpenArchive();
		}
		if (GetKeyState(VK_CONTROL) & 0x0800 && virtualKey == 0x46) // [CTRL] + [o]
		{
			(void)OpenFolder();
		}
	}

	if (virtualKey == 0x5a) // z
	{
		try
		{

			mAnimImage.Load(IDR_RCDATA1, mGraphicManager.RenderTarget());
			SetTimer(m_hwnd, 0, 60, nullptr);

		}
		catch (std::runtime_error& exc)
		{
			MessageBox(m_hwnd, ToWideString(exc.what()).c_str(), L"Exception", MB_OK | MB_ICONEXCLAMATION);
		}
	}
	if (virtualKey == 0x48)  // h
	{

	}
	if (virtualKey == 0x4c) // l
	{

	}

	if (virtualKey == 0x42) // b
	{
		mBossMode.SetActive(!mBossMode.IsActive());
		mAnimImage.SetLoaded(false);
		KillTimer(m_hwnd, 0);
	}

	InvalidateRect(m_hwnd, nullptr, true);
}

HRESULT Viewer::OpenFolder()
{
	IFileDialog* pFileDialog = nullptr;
	std::wstringstream selectedFolder{};

	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog));
	if (SUCCEEDED(hr))
	{
		DWORD dwOptions;
		pFileDialog->GetOptions(&dwOptions);
		pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);

		if (SUCCEEDED(pFileDialog->Show(nullptr)))
		{
			IShellItem* pSelectedItem = nullptr;
			if (SUCCEEDED(pFileDialog->GetResult(&pSelectedItem)))
			{
				PWSTR pszFilePath = nullptr;
				if (SUCCEEDED(pSelectedItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
				{
					selectedFolder << pszFilePath;
					CoTaskMemFree(pszFilePath);
				}
				pSelectedItem->Release();
			}
		}
		pFileDialog->Release();
	}
	std::vector<std::wstring> images{};

	if (!selectedFolder.str().empty())
	{
		WIN32_FIND_DATAW data{};
		HANDLE hSearch = FindFirstFileW((L"\\\\?\\" + selectedFolder.str() + L"\\*").c_str(), &data);
		do {
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				continue; // skip directories for now, else we have to look if the cFilename == '.' || '..'
			}
			m_files.emplace_back(selectedFolder.str() + std::wstring{ data.cFileName });

			OutputDebugStringW((L"Found " + std::wstring{ data.cFileName } + L"\n").c_str());
		} while (hSearch != INVALID_HANDLE_VALUE && FindNextFileW(hSearch, &data));
	}



	return hr;
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
		std::unique_ptr<ZipFile> const& item = mZipManager.Current();
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

void Viewer::OnTimer() noexcept
{
	mAnimImage.Update();
	InvalidateRect(m_hwnd, nullptr, TRUE);
}

void Viewer::OnDpiChanged(int x, int y, RECT rct) noexcept
{
	// SetWindowPos(m_hwnd, nullptr, )
	std::wstringstream out;
	out << L"OnDpiChanged " << x << L"," << y << "\n";
	LOG(out.str().c_str());

	SetWindowPos(
		m_hwnd, nullptr,
		rct.left, rct.top,
		rct.right - rct.left,
		rct.bottom - rct.top,
		SWP_NOZORDER | SWP_NOACTIVATE
	);

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

void Viewer::UpdateTitle() const
{
	std::wstringstream title;
	title << TITLE << L" "
		<< (mCurrentPage + 1) << L"/" << mZipManager.Size();

	SetWindowText(m_hwnd, title.str().c_str());

}

void Viewer::ResetTitle() const
{
	SetWindowTextW(m_hwnd, TITLE);
}

void Viewer::ArchiveWorker(Viewer* viewer, std::wstring const& Filename)
{
	// This could be a problem, for now its detached and it works for my use case.
	// Could be considered bad practice, but its a background task that will be ready when its ready.
	// 2023-11-10: I think I should look into CRITICAL_SECTION to mark it.
	LOG(L"ArchiveWorker %s\n", Filename.c_str());
	viewer->mZipManager.ReadZip(Filename);
	viewer->m_imageX = viewer->m_imageY = 0;
	HRESULT hr = viewer->LoadImage();
	if (FAILED(hr))
	{
		LOG(L"Failed to load picture\n");
	}
	viewer->UpdateTitle();

}
