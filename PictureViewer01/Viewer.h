#pragma once
#include "Application.h"
#include "BaseWindow.h"
#include "SafeRelease.h"
#include <memory>

#include <dwrite.h>
#include <commdlg.h>

constexpr wchar_t PICTURE[] = L"C:\\Code\\digits.png";



struct ZipMemory
{
	void* GlobalData;
	HGLOBAL Global;

};

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
		OutputDebugStringW(L"ZipFile destructor\n");
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

		return hr;
	}

	HRESULT RecreateStream()
	{
		SafeRelease(Stream);
		return CreateStreamOnHGlobal(Global, false, &Stream);
	}
};

struct ZipList
{
	std::vector<ZipFile> m_list;
	std::vector<ZipMemory> m_memory_list;


};
std::wstring ToWideString(std::string const& Text);
std::string FromWideString(std::wstring const& Text);
std::vector<std::shared_ptr<ZipFile>> ReadZip(std::wstring const& Filename);

class Viewer : public BaseWindow<Viewer>
{
public:
	Viewer();
	~Viewer();

	HRESULT Initialize(HINSTANCE hInst);
	HRESULT CreateDeviceResources(HWND hwnd);
	HRESULT LoadFile(std::wstring const& Path);
	HRESULT LoadImage(int delta) ;
	HRESULT OpenArchive();

	virtual void OnSize(UINT Width, UINT Height) noexcept override;
	virtual LRESULT OnPaint(HWND hwnd) noexcept override;
	virtual void OnKeyDown(UINT32 VirtualKey) noexcept override;
	virtual void OnMouseMove(MouseMoveControl ctrl, float x, float y) noexcept override;
	virtual void OnLButtonDown(float x, float y) noexcept override;
	virtual void OnLButtonUp(float x, float y) noexcept override;
	virtual void OnMouseScrollWheel(short delta) noexcept override;

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

	int m_currentPage = -1;
	std::vector<std::shared_ptr<ZipFile>> m_zip_files;
	std::vector<HGLOBAL> m_zip_globals;

};

