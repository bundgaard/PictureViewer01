#pragma once

#include <vector>
#include <string>
#include "GraphicFactory.h"

class GraphicsManager;
class AnimatedImage
{
	GraphicFactory& mGraphicsFactory;
	unsigned int mFrameCount;
	std::vector<IWICBitmapFrameDecode*> mFrames;
	
public:
	AnimatedImage(GraphicFactory& graphicsFactory);
	~AnimatedImage();

	void Load(std::wstring const& filepath);
	void Render();
	void Update();
};

