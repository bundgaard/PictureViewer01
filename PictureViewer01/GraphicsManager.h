#pragma once

#include <Windows.h>
#include "SafeRelease.h"
#include <string>
#include <d2d1.h>
#include <memory>


struct IWICFormatConverter;
struct ID2D1HwndRenderTarget;
struct ID2D1Bitmap;
struct ID2D1SolidColorBrush;
struct IDWriteTextFormat;
/*
Graphic Manager should become free standing, meaning that things that all need can use GraphicManager and things that are device specific should have another class taking GraphicManager.


Now this has 2 responsibilities in the class, Create the factories and create the device specific things.

I can shove this around and share it with many but only 1 window can actually own this object.
This needs to be rectified so we can create this in all classes and each have their own graphic manager with their device specific settings.

Now because I am trying to separate out the continous images from the static images, I might need a new Window which means a new HWND which means a separate GraphicManager with its own HWNDRENDERTARGET.

I guess a battleplan would be to separate out the Graphic Manager to two classes.
1 class with factories.
1 class with device specific things.

GraphicsManager will become factory for Direct2D and its affiliated members.
*/
class GraphicFactory;
class GraphicsManager
{
	GraphicFactory& mGraphicFactory;

	IWICFormatConverter* m_wic_converter = nullptr;
	IDWriteTextFormat* m_textFormat = nullptr;

	ID2D1HwndRenderTarget* m_renderTarget = nullptr; // should be moved
	ID2D1Bitmap* m_bitmap = nullptr; // should be moved
	ID2D1SolidColorBrush* m_brush = nullptr; // should be moved
	HWND mHwnd; // should be moved
public:
	GraphicsManager(GraphicFactory& factory);
	~GraphicsManager();

	void Initialize(HWND hwnd); // should be moved
	void ReleaseConverter(); // should be moved
	void ReleaseDeviceResources(); // should be moved
	void ReleaseBitmap();// should be moved

	HRESULT CreateBitmapFromWicBitmap(); // should be moved
	HRESULT CreateDeviceResources(HWND hwnd); // should be moved
	HRESULT CreateFormatConverter(); // should be moved
	HRESULT CreateBitmapFromIStream(IStream* pStream);// should be moved
	HRESULT CreateBitmapFromFile(std::wstring const& Filepath); // should be moved
	void Resize(int Width, int Height); // should be moved
	void DrawText(std::wstring const& Text, float x, float y, D2D1::ColorF color = D2D1::ColorF::Black); // should be moved
	void DrawTextCentered(std::wstring const& Text, float y, D2D1::ColorF color = D2D1::ColorF::Black); // should be moved
	D2D1_WINDOW_STATE CheckWindowState() const; // should be moved

	ID2D1SolidColorBrush* Brush(); // should be moved
	ID2D1HwndRenderTarget* RenderTarget(); // should be moved
	ID2D1Bitmap* Bitmap(); // should be moved
	IDWriteTextFormat* TextFormat(); // should be moved
	IWICFormatConverter* Converter();


};

