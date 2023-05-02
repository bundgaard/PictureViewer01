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

class Viewer : public BaseWindow<Viewer>
{
public:
	Viewer();
	~Viewer();

	HRESULT Initialize(HINSTANCE hInst);
	HRESULT CreateDeviceResources(HWND hwnd);
	

	HRESULT LoadFile(std::wstring const& Path);

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

	float m_lastMouseX{};
	float m_lastMouseY{};
	float m_imageX{};
	float m_imageY{};
	float m_scaleFactor = 1.0;

};

