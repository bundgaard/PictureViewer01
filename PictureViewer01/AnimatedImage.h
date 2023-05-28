#pragma once

#include <vector>
#include <objidl.h>
#include <wincodec.h>
#include <memory>
#include <string>

class GraphicsManager;
class AnimatedImage
{
	HWND mWnd;
	std::unique_ptr<GraphicsManager>& mGraphicsManager;
	unsigned int mFrameCount;
	std::vector<IWICBitmapFrameDecode*> mFrames;
	static LRESULT CALLBACK sWndProc(HWND, UINT, WPARAM, LPARAM);

public:
	AnimatedImage(const HWND hwnd, std::unique_ptr<GraphicsManager>& graphicsManager);
	~AnimatedImage();

	void Load(std::wstring const& filepath);

};

