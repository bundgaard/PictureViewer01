#include "Viewer.h"
#include <windowsx.h>

#include <sstream>

Viewer::Viewer() :
	m_bitmap(nullptr),
	m_wic_factory(nullptr),
	m_d2_factory(nullptr),
	m_brush(nullptr),
	m_renderTarget(nullptr),
	m_currentPage(-1)
{
}

Viewer::~Viewer()
{
	SafeRelease(m_brush);
	SafeRelease(m_bitmap);
	SafeRelease(m_d2_factory);
	SafeRelease(m_renderTarget);

	SafeRelease(m_wic_factory);
	SafeRelease(m_wic_converter);

	SafeRelease(m_textFormat);
	SafeRelease(m_dwrite_factory);
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
		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, _uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_dwrite_factory));
	}

	if (SUCCEEDED(hr))
	{
		hr = m_dwrite_factory->CreateTextFormat(
			L"Comic Sans MS",
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			18.0f,
			L"",
			&m_textFormat);
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
			D2D1_RENDER_TARGET_PROPERTIES renderProperties = D2D1::RenderTargetProperties();
			renderProperties.dpiX = 96.0f;
			renderProperties.dpiY = 96.0f;

			auto hwndProperties = D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU((rc.right - rc.left), (rc.bottom - rc.top)));

			hr = m_d2_factory->CreateHwndRenderTarget(
				renderProperties,
				hwndProperties,
				&m_renderTarget);
			if (SUCCEEDED(hr))
			{
				hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_brush);
			}
		}
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
			std::wstring PressSpaceText = L"Press [SPACE BAR] to show the picture.";
			IDWriteTextLayout* layout = nullptr;
			if (SUCCEEDED(hr))
			{
				hr = m_dwrite_factory->CreateTextLayout(PressSpaceText.c_str(), static_cast<UINT32>(PressSpaceText.size()), m_textFormat, 320.f, 240.f, &layout);
			}
			DWRITE_TEXT_METRICS metric{};
			if (SUCCEEDED(hr))
			{
				hr = layout->GetMetrics(&metric);
			}

			if (SUCCEEDED(hr))
			{
				auto color = m_brush->GetColor();
				m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
				m_renderTarget->DrawTextLayout(D2D1::Point2F((640.0f - metric.width) / 2.0f, (480.0f - metric.height) / 2.0f), layout, m_brush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
				m_brush->SetColor(color);
			}
			SafeRelease(layout);
			if (m_bitmap)
			{
				auto ClientSize = m_renderTarget->GetSize();
				auto BitmapSize = m_bitmap->GetSize();
				auto WidthRatio = std::abs(BitmapSize.width / ClientSize.width); // TODO need to fix this so it doesn't scale when resizing app
				auto HeightRatio = std::abs(BitmapSize.height / ClientSize.height);
				auto Ratio = GetImageRatio(BitmapSize.width, BitmapSize.height);
				Ratio *= m_scaleFactor;
				auto ClientRect = D2D1::RectF(m_imageX, m_imageY, m_imageX + BitmapSize.width, m_imageY + BitmapSize.height);
				D2D1_MATRIX_3X2_F transform{};
				m_renderTarget->GetTransform(&transform);
				m_renderTarget->SetTransform(D2D1::Matrix3x2F::Scale(m_scaleFactor, m_scaleFactor));
				m_renderTarget->DrawBitmap(m_bitmap, ClientRect);
				m_renderTarget->SetTransform(transform);
			}
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
	if (VirtualKey == VK_PRIOR)
	{

		HRESULT hr = this->LoadImage(-1);
		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Loaded image\n");
		}
		// PageUp
	}
	if (VirtualKey == VK_NEXT)
	{
		// Page Down
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
		SafeRelease(m_wic_converter);
		SafeRelease(m_bitmap);
	}
	InvalidateRect(m_hwnd, nullptr, true);
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

HRESULT Viewer::LoadImage(int delta)
{
	HRESULT hr = S_OK;
	IWICBitmapDecoder* decoder = nullptr;

	SafeRelease(m_bitmap);
	SafeRelease(m_wic_converter);

	if (SUCCEEDED(hr))
	{
		hr = m_zip_files.size() > 0 ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		std::wstringstream Out;
		Out << L"Zip files " << m_zip_files.size() << L"\n";
		Out << L"Delta " << delta << L"\n";
		
		OutputDebugStringW(Out.str().c_str());

		if (m_currentPage + delta < 0)
		{
			m_currentPage = m_zip_files.size()-1;
		}
		else if (m_currentPage + delta > m_zip_files.size())
		{
			m_currentPage = 0;
		}
		else
		{
			Out.clear();
			Out << L"Current page " << m_currentPage << L" delta " << delta << L" ";
			m_currentPage += delta;
			Out << m_currentPage << L"\n";
		}


		Out.clear();
		Out << L"Current page" << m_currentPage << L"\n";
		OutputDebugStringW(Out.str().c_str());
		std::shared_ptr<ZipFile> item = m_zip_files.at(m_currentPage);
		Out.clear();
		Out << L"Name " << ToWideString(item->Name) << L"\n";
		OutputDebugString(Out.str().c_str());

		OutputDebugStringW(L"Create decoder from stream\n");
		hr = item->RecreateStream();
		if (SUCCEEDED(hr))
		{
			hr = m_wic_factory->CreateDecoderFromStream(
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
		SafeRelease(m_wic_converter);
		hr = m_wic_factory->CreateFormatConverter(&m_wic_converter);
	}

	if (SUCCEEDED(hr))
	{
		OutputDebugStringW(L"Create format converter\n");
		hr = m_wic_converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
	}

	if (SUCCEEDED(hr))
	{
		OutputDebugStringW(L"Initialized converter\n");
		hr = CreateDeviceResources(m_hwnd);
	}

	if (SUCCEEDED(hr))
	{
		OutputDebugStringW(L"Create device resources\n");
		SafeRelease(m_bitmap);
		hr = m_renderTarget->CreateBitmapFromWicBitmap(m_wic_converter, &m_bitmap);
	}

	SafeRelease(decoder);
	SafeRelease(frame);
	return hr;
}

void Viewer::OnSize(UINT Width, UINT Height) noexcept
{
	OutputDebugStringW(L"Received WM_SIZE\n");
	wchar_t Buf[64] = { 0 };
	swprintf(Buf, 64, L"%u,%u\n", Width, Height);
	OutputDebugStringW(Buf);
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

std::wstring ToWideString(std::string const& Text)
{

	auto const Size = MultiByteToWideChar(CP_UTF8, 0, Text.c_str(), Text.size(), nullptr, 0);
	std::wstring Result;
	Result.resize(Size);
	MultiByteToWideChar(CP_UTF8, 0, Text.c_str(), Text.size(), Result.data(), Result.size());
	return Result;
}
std::string FromWideString(std::wstring const& Text)
{
	auto const Size = WideCharToMultiByte(CP_UTF8, 0, Text.c_str(), static_cast<int>(Text.size()), nullptr, 0, nullptr, nullptr);
	std::string Result;
	Result.resize(Size);
	WideCharToMultiByte(CP_UTF8, 0, Text.c_str(), static_cast<int>(Text.size()), Result.data(), Result.size(), nullptr, nullptr);
	return Result;
}

std::vector<std::shared_ptr<ZipFile>> ReadZip(std::wstring const& Filename)
{
	std::vector<std::shared_ptr<ZipFile>> Files;

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
			std::shared_ptr<ZipFile> ptr = std::make_unique<ZipFile>(stat.name, static_cast<size_t>(stat.size));
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