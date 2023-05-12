#include "Viewer.h"
#include <windowsx.h>

#include <zip.h>

#include <sstream>
#include "BaseWindow.h"
#include "SafeRelease.h"
#include "ZipFile.h"
#include "Converter.h"
#include "GraphicsManager.h"

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
	if (BeginPaint(hwnd, &ps))
	{
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

			mGraphicManager.RenderTarget()->DrawLine(D2D1::Point2F(0.0f, 0.0f), D2D1::Point2F(100.0f, 100.0f), mGraphicManager.Brush());

			if (mGraphicManager.Converter() && !mGraphicManager.Bitmap())
			{
				auto ptr = mGraphicManager.Bitmap();
				mGraphicManager.RenderTarget()->CreateBitmapFromWicBitmap(mGraphicManager.Converter(), &ptr);
			}
			std::wstring PressSpaceText = L"Press [SPACE BAR] to show the picture.";
			IDWriteTextLayout* layout = nullptr;
			if (SUCCEEDED(hr))
			{
				// TODO: move into GraphicManager
				hr = mGraphicManager.WriteFactory()->CreateTextLayout(PressSpaceText.c_str(), static_cast<UINT32>(PressSpaceText.size()), mGraphicManager.TextFormat(), 320.f, 240.f, &layout);
			}
			DWRITE_TEXT_METRICS metric{};
			if (SUCCEEDED(hr))
			{
				hr = layout->GetMetrics(&metric);
			}

			if (SUCCEEDED(hr))
			{
				auto color = mGraphicManager.Brush()->GetColor();
				mGraphicManager.Brush()->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
				mGraphicManager.RenderTarget()->DrawTextLayout(D2D1::Point2F((640.0f - metric.width) / 2.0f, (480.0f - metric.height) / 2.0f), layout, mGraphicManager.Brush(), D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
				mGraphicManager.Brush()->SetColor(color);
			}
			SafeRelease(layout);
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
		EndPaint(hwnd, &ps);
	}
	return SUCCEEDED(hr) ? S_OK : E_FAIL;
}

void Viewer::OnKeyDown(UINT32 VirtualKey) noexcept
{
	if (VirtualKey == VK_SPACE)
	{
		m_imageX = m_imageY = 0;
		m_scaleFactor = 1.0f;
		m_zip_files.clear();
		m_zip_files = ReadZip(L"C:\\Code\\pics.zip");
		HRESULT hr = this->LoadFile(PICTURE);
		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Loaded file\n");
		}
		m_currentPage = -1;

	}
	if (VirtualKey == VK_PRIOR) // PageUp
	{

		HRESULT hr = this->LoadImage(-1);
		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Loaded image\n");
		}
		
	}
	
	if (VirtualKey == VK_NEXT) // Page Down
	{
		
		HRESULT hr = this->LoadImage(+1);
		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Loaded image\n");
		}
	}

	if (VirtualKey == VK_ESCAPE)
	{
		OutputDebugStringW(L"ESCAPE pressed\n");
		m_imageX = m_imageY = 0;
		m_zip_files.clear();
		mGraphicManager.ReleaseConverter();
		mGraphicManager.ReleaseDeviceResources();
	}
	if ((wchar_t)VirtualKey == L'o')
	{
		OutputDebugStringW(L"Keydown test for ctrl + o\n");
	}
	InvalidateRect(m_hwnd, nullptr, true);
}


HRESULT Viewer::LoadFile(std::wstring const& Path)
{
	HRESULT hr = S_OK;
	IWICBitmapDecoder* decoder = nullptr;
	if (SUCCEEDED(hr))
	{
		hr = mGraphicManager.WICFactory()->CreateDecoderFromFilename(Path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
	}

	IWICBitmapFrameDecode* frame = nullptr;
	if (SUCCEEDED(hr))
	{
		hr = decoder->GetFrame(0, &frame);
	}

	if (SUCCEEDED(hr))
	{
		
		hr = mGraphicManager.CreateFormatConverter();
	}

	if (SUCCEEDED(hr))
	{
		hr = mGraphicManager.Converter()->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
	}

	if (SUCCEEDED(hr))
	{
		hr = mGraphicManager.CreateDeviceResources(m_hwnd);
	}

	if (SUCCEEDED(hr))
	{
		hr = mGraphicManager.CreateBitmapFromWicBitmap();
	}

	SafeRelease(decoder);
	SafeRelease(frame);

	return hr;
}

HRESULT Viewer::LoadImage(int delta)
{
	HRESULT hr = S_OK;
	IWICBitmapDecoder* decoder = nullptr;

	mGraphicManager.ReleaseBitmap();
	mGraphicManager.ReleaseConverter();

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
		
		OutputDebugStringW(L"Create decoder from stream\n");
		hr = item->RecreateStream();
		if (SUCCEEDED(hr))
		{
			hr = mGraphicManager.WICFactory()->CreateDecoderFromStream(
				item->Stream,
				nullptr,
				WICDecodeMetadataCacheOnDemand,
				&decoder);
		}

	}

	IWICBitmapFrameDecode* frame = nullptr;
	if (SUCCEEDED(hr))
	{
		OutputDebugStringW(L"Created decoder from Stream\n");
		hr = decoder->GetFrame(0, &frame);
	}

	if (SUCCEEDED(hr))
	{
		OutputDebugStringW(L"GetFrame\n");
		mGraphicManager.CreateFormatConverter();
	}

	if (SUCCEEDED(hr))
	{
		OutputDebugStringW(L"Create format converter\n");
		hr = mGraphicManager.Converter()->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
	}

	if (SUCCEEDED(hr))
	{
		OutputDebugStringW(L"Initialized converter\n");
		hr = mGraphicManager.CreateDeviceResources(m_hwnd);
	}

	if (SUCCEEDED(hr))
	{
		OutputDebugStringW(L"Create device resources\n");
		mGraphicManager.CreateBitmapFromWicBitmap();
		
	}

	SafeRelease(decoder);
	SafeRelease(frame);
	return hr;
}

HRESULT Viewer::OpenArchive()
{

	// openFile dialog with only zip archive.
	OPENFILENAME ofn;

	return E_NOTIMPL;
}

void Viewer::OnSize(UINT Width, UINT Height) noexcept
{
	OutputDebugStringW(L"Received WM_SIZE\n");
	wchar_t Buf[64] = { 0 };
	swprintf(Buf, 64, L"%u,%u\n", Width, Height);
	OutputDebugStringW(Buf);
	if (mGraphicManager.RenderTarget() != nullptr)
	{
		if (FAILED(mGraphicManager.RenderTarget()->Resize(D2D1::SizeU(Width, Height))))
		{
			mGraphicManager.ReleaseDeviceResources();
		}
	}
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
	wchar_t Buf[64] = { 0 };
	swprintf(Buf, 64, L"Zoom: %f\n", (1.0f - m_scaleFactor) * 100.f);
	OutputDebugStringW(Buf);
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
		OutputDebugStringW(L"Failed to open zip archive\n");
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