#pragma once
#include "Application.h"
#include "BaseWindow.h"

#include <dwrite.h>
constexpr wchar_t PICTURE[] = L"C:\\Code\\digits.png";

template<typename T>
inline void SafeRelease(T*& t)
{
	if (NULL != t)
	{
		t->Release();
		t = nullptr;
	}
}

struct ZipFile
{
	std::string Name;
	size_t Size;
	void* GlobalData;
	HGLOBAL Global;
	IStream *Stream;

	ZipFile(std::string const name, size_t size) :
		Name(name),
		Size(size),
		GlobalData(nullptr),
		Global(nullptr),
		Stream(nullptr)
	{

	}

	~ZipFile()
	{

		if (Global)
			GlobalFree(Global);
		SafeRelease(Stream);
	}

	HRESULT Write(std::vector<byte>&& Bytes)
	{
		HRESULT hr = S_OK;
		if (SUCCEEDED(hr))
		{
			Global = GlobalAlloc(GMEM_MOVEABLE, Bytes.size());
			hr = Global ? S_OK : E_FAIL;
		}
		if (SUCCEEDED(hr))
		{
			GlobalData = GlobalLock(Global);
			hr = GlobalData ? S_OK : E_FAIL;
		}

		if (SUCCEEDED(hr))
		{
			CopyMemory(GlobalData, Bytes.data(), Bytes.size());
			hr = GlobalUnlock(GlobalData) ? S_OK : E_FAIL;
		}

		if (SUCCEEDED(hr))
		{
			hr = CreateStreamOnHGlobal(Global, false, &Stream);
		}

		return hr;
	}
};

std::string FromWideString(std::wstring const& Text);
std::vector<ZipFile> ReadZip(std::wstring const& Filename);

class Viewer : public BaseWindow<Viewer>
{
public:
	Viewer();
	~Viewer();

	HRESULT Initialize(HINSTANCE hInst);
	HRESULT CreateDeviceResources(HWND hwnd);
	HRESULT LoadFile(std::wstring const& Path);
	HRESULT LoadImage(int delta) noexcept;

	virtual void OnSize(UINT Width, UINT Height) noexcept override;
	virtual LRESULT OnPaint(HWND hwnd) noexcept override;
	virtual void OnKeyDown(UINT32 VirtualKey) noexcept override;
	virtual void OnMouseMove(MouseMoveControl ctrl, float x, float y) noexcept override;
	virtual void OnLButtonDown(float x, float y) noexcept override;
	virtual void OnLButtonUp(float x, float y) noexcept override;
	virtual void OnMouseScrollWheel(short delta) noexcept override;

	void NextImage() noexcept;
	void PreviousImage() noexcept;


private:
	IWICImagingFactory* m_wic_factory = nullptr;
	IWICFormatConverter* m_wic_converter = nullptr;

	ID2D1Factory* m_d2_factory = nullptr;
	ID2D1HwndRenderTarget* m_renderTarget = nullptr;
	ID2D1Bitmap* m_bitmap = nullptr;
	ID2D1SolidColorBrush* m_brush = nullptr;

	IDWriteFactory* m_dwrite_factory = nullptr;
	IDWriteTextFormat* m_textFormat = nullptr;

	float m_lastMouseX{};
	float m_lastMouseY{};
	float m_imageX{};
	float m_imageY{};
	float m_scaleFactor = 1.0;

	size_t m_currentPage{};
	std::vector<ZipFile> m_zip_files;

};

