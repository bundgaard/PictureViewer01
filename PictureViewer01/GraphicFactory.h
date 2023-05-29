#pragma once
#include <memory>

struct IWICImagingFactory;
struct ID2D1Factory;
struct IDWriteFactory;

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

