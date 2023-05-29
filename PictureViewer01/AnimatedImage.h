#pragma once

#include <vector>
#include <objidl.h>
#include <wincodec.h>
#include <memory>
#include <string>

class GraphicsManager;
class GraphicFactory;
class AnimatedImage
{
	HWND mWnd;
	GraphicFactory& mGraphicsFactory;
	std::unique_ptr<GraphicsManager> mGraphicsManager;
	unsigned int mFrameCount;
	std::vector<IWICBitmapFrameDecode*> mFrames;
	static LRESULT CALLBACK sWndProc(HWND, UINT, WPARAM, LPARAM);

public:
	AnimatedImage(const HWND hwnd, GraphicFactory& graphicsFactory);
	~AnimatedImage();

	void Load(std::wstring const& filepath);

};

