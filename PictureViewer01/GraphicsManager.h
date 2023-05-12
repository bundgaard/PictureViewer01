#pragma once

#include <Windows.h>
#include "SafeRelease.h"
#include <string>
#include <d2d1.h>

struct IWICImagingFactory;
struct IWICFormatConverter;
struct ID2D1Factory;
struct ID2D1HwndRenderTarget;
struct ID2D1Bitmap;
struct ID2D1SolidColorBrush;
struct IDWriteFactory;
struct IDWriteTextFormat;

class GraphicsManager
{
	IWICImagingFactory* m_wic_factory = nullptr;
	IWICFormatConverter* m_wic_converter = nullptr;

	ID2D1Factory* m_d2_factory = nullptr;
	ID2D1HwndRenderTarget* m_renderTarget = nullptr;
	ID2D1Bitmap* m_bitmap = nullptr;
	ID2D1SolidColorBrush* m_brush = nullptr;

	IDWriteFactory* m_dwrite_factory = nullptr;
	IDWriteTextFormat* m_textFormat = nullptr;
	HWND mHwnd;
public:
	GraphicsManager();
	~GraphicsManager();
	
	void Initialize(HWND hwnd);
	void ReleaseConverter();
	void ReleaseDeviceResources();
	void ReleaseBitmap();

	HRESULT CreateBitmapFromWicBitmap();
	HRESULT CreateDeviceResources(HWND hwnd);
	HRESULT CreateFormatConverter();
	HRESULT CreateBitmapFromIStream(IStream* pStream);
	HRESULT CreateBitmapFromFile(std::wstring const& Filepath);
	
	void Resize(int Width, int Height);

	D2D1_WINDOW_STATE CheckWindowState() const;

	ID2D1SolidColorBrush* Brush();

	ID2D1HwndRenderTarget* RenderTarget();
	ID2D1Bitmap* Bitmap();

	IDWriteTextFormat* TextFormat();

	IDWriteFactory* WriteFactory();

	IWICImagingFactory* WICFactory();
	IWICFormatConverter* Converter();


};

