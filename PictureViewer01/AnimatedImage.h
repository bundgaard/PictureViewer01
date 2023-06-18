#pragma once

#include <vector>
#include <string>
#include "GraphicFactory.h"

class GraphicsManager;
class AnimatedImage
{
	GraphicFactory& mGraphicsFactory;
	
	unsigned int mFrameCount;
	
	bool mLoaded = false;
	
	ID2D1Bitmap* mCurrentFrame = nullptr;
	int mCurrentFrameIdx = 0;
	std::vector<ID2D1Bitmap*> mFrames;
	void DrawBackground(ID2D1HwndRenderTarget* renderTarget);
public:
	AnimatedImage(GraphicFactory& graphicsFactory);
	~AnimatedImage();

	void Load(std::wstring const& filepath, ID2D1HwndRenderTarget* renderTarget);
	bool IsLoaded();
	void SetLoaded(bool aValue);
	void Render(ID2D1HwndRenderTarget* renderTarget);
	void Update();
};

