
#include "Viewer.h"
#include <windowsx.h>

#include <zip.h>

#include <sstream>
#include <array>

#include "BaseWindow.h"
#include "SafeRelease.h"
#include "ZipFile.h"
#include "Converter.h"
#include "GraphicsManager.h"

void Log(const wchar_t* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	std::wstring Text;
	Text.resize(_vscwprintf(fmt, args));
	_vsnwprintf_s(Text.data(), Text.size(), _TRUNCATE, fmt, args);
	OutputDebugStringW(Text.c_str());
	va_end(args);
}

#ifdef _DEBUG
#define LOG(fmt, ...) Log(fmt, __VA_ARGS__)
#else 
#define LOG(fmt, ...)
#endif

Viewer::Viewer(GraphicsManager& graphicManager)
	: mGraphicManager(graphicManager)
	, m_currentPage(-1)
{
}

Viewer::~Viewer()
{
	OutputDebugStringW(L"Viewer DTOR\n");
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
		wc.lpszClassName = L"CPICTUREVIEWER01";
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
		mGraphicManager.Initialize(hwnd);
		hr = hwnd ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		OutputDebugStringW(L"Created Window\n");
	}
	return hr;
}



inline float GetImageRatio(float width, float height)
{
	auto Big = std::max(width, height);
	auto Small = std::min(width, height);
	auto Ratio = std::abs(Big / Small);
	return Ratio;
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

		auto color = mGraphicManager.Brush()->GetColor();
		mGraphicManager.Brush()->SetColor(D2D1::ColorF(D2D1::ColorF::PaleVioletRed));
		mGraphicManager.RenderTarget()->FillRectangle(D2D1::RectF(0.f, 0.f, 640.f, 480.f), mGraphicManager.Brush());
		mGraphicManager.Brush()->SetColor(color);

		if (mGraphicManager.Converter() && !mGraphicManager.Bitmap())
		{
			auto ptr = mGraphicManager.Bitmap();
			mGraphicManager.RenderTarget()->CreateBitmapFromWicBitmap(mGraphicManager.Converter(), &ptr);
		}

		std::wstring OpenFileText = L"[CTRL] + [o] - To open archive.";
		mGraphicManager.DrawTextCentered(OpenFileText, 50);
		mGraphicManager.DrawTextCentered(L"PageUp and PageDown to move back and forth between images in archive.", 75);

		if (mGraphicManager.Bitmap())
		{
			auto ClientSize = mGraphicManager.RenderTarget()->GetSize();
			auto BitmapSize = mGraphicManager.Bitmap()->GetSize();

			auto WidthRatio = std::abs(BitmapSize.width / ClientSize.width); // TODO need to fix this so it doesn't scale when resizing app
			auto HeightRatio = std::abs(BitmapSize.height / ClientSize.height);
			
			auto Ratio = GetImageRatio(BitmapSize.width, BitmapSize.height);
			
			Ratio *= m_scaleFactor;
			
			auto ClientRect = D2D1::RectF(m_imageX, m_imageY, m_imageX + BitmapSize.width, m_imageY + BitmapSize.height);
			
			D2D1_MATRIX_3X2_F transform{};
			
			mGraphicManager.RenderTarget()->GetTransform(&transform);
			mGraphicManager.RenderTarget()->SetTransform(D2D1::Matrix3x2F::Scale(m_scaleFactor, m_scaleFactor));
			mGraphicManager.RenderTarget()->DrawBitmap(mGraphicManager.Bitmap(), ClientRect);
			mGraphicManager.RenderTarget()->SetTransform(transform);
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
#ifdef DEBUG || _DEBUG

	std::wstringstream Out;
	Out << std::hex << VirtualKey << L"\n";
	OutputDebugStringW(Out.str().c_str());
#endif
	//	if (VirtualKey == VK_SPACE)
	//	{
	//		m_imageX = m_imageY = 0;
	//		m_scaleFactor = 1.0f;
	//		m_zip_files.clear();
	//#if 0
	//		m_zip_files = ReadZip(L"C:\\Temp\\7901387a-b652-496f-b378-08c69c34f88f.zip"); //  ReadZip(L"C:\\Code\\pics.zip");
	//#else
	//		m_zip_files = ReadZip(L"C:\\Code\\pics.zip");
	//#endif
	//		
	//		HRESULT hr = this->LoadFile(PICTURE);
	//		if (SUCCEEDED(hr))
	//		{
	//			Log(L"Loaded file\n");
	//		}
	//		m_currentPage = -1;
	//
	//	}
	if (VirtualKey == VK_PRIOR) // PageUp
	{

		HRESULT hr = this->LoadImage(-1);
		if (SUCCEEDED(hr))
		{
			LOG(L"Loaded image\n");
		}

	}

	if (VirtualKey == VK_NEXT) // Page Down
	{

		HRESULT hr = this->LoadImage(+1);
		if (SUCCEEDED(hr))
		{
			LOG(L"Loaded image\n");
		}
	}

	if (VirtualKey == VK_ESCAPE)
	{
		LOG(L"ESCAPE pressed\n");
		m_imageX = m_imageY = 0;
		m_zip_files.clear();
		mGraphicManager.ReleaseConverter();
		mGraphicManager.ReleaseDeviceResources();
	}


	if (GetKeyState(VK_CONTROL) & 0x0800 && VirtualKey == 0x4f) // o
	{
		HRESULT hr = OpenArchive();
		if (SUCCEEDED(hr))
		{
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
		hr = m_zip_files.size() > 0 ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{
		m_currentPage += delta;


		if (m_currentPage < 0)
		{
			m_currentPage = static_cast<int>(m_zip_files.size()) - 1;
		}
		else if (m_currentPage >= static_cast<int>(m_zip_files.size()))
		{
			m_currentPage = 0;
		}


		std::unique_ptr<ZipFile>& item = m_zip_files.at(m_currentPage);

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

		m_imageX = m_imageY = 0;
		m_scaleFactor = 1.0f;
		m_zip_files.clear();
		m_zip_files = ReadZip(ofn.lpstrFile);
		m_currentPage = -1;
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



std::vector<std::unique_ptr<ZipFile>> Viewer::ReadZip(std::wstring const& Filename)
{
	std::vector<std::unique_ptr<ZipFile>> Files;

	zip* archive = zip_open(FromWideString(Filename).c_str(), 0, nullptr);
	if (!archive)
	{
		Log(L"Failed to open zip archive\n");
		return {};
	}

	auto NumFiles = zip_get_num_files(archive);
	Files.reserve(NumFiles);

	for (int i = 0; i < NumFiles; i++)
	{
		struct zip_stat stat;
		zip_stat_index(archive, i, 0, &stat);

		zip_file* File = zip_fopen_index(archive, i, 0);
		if (File)
		{
			std::unique_ptr<ZipFile> ptr = std::make_unique<ZipFile>(stat.name, static_cast<size_t>(stat.size));
			std::vector<byte> Bytes;
			Bytes.resize(ptr->Size);
			zip_int64_t bytes_read = zip_fread(File, Bytes.data(), Bytes.size());
			HRESULT hr = S_OK;
			hr = bytes_read == ptr->Size ? S_OK : E_FAIL;
			if (SUCCEEDED(hr))
			{
				hr = ptr->Write(std::move(Bytes));
			}

			if (SUCCEEDED(hr))
			{
				Files.push_back(std::move(ptr));
				zip_fclose(File);
			}
		}
	}
	zip_close(archive);
	return Files;
}