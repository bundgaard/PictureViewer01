#pragma once
#include <memory>
#include <dwrite.h>
#include <d2d1.h>
#include <wincodec.h>

class GraphicFactory
{
	ID2D1Factory* mD2Factory;
	IDWriteFactory* mWriteFactory;
	IWICImagingFactory* mWICFactory;

public:
	GraphicFactory();
	~GraphicFactory();

	ID2D1Factory* GetD2Factory();
	IDWriteFactory* GetWriteFactory();
	IWICImagingFactory* GetWICFactory();
};

