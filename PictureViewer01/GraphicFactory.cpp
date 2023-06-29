#include "GraphicFactory.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>

#include "Log.h"

GraphicFactory::GraphicFactory()
	: mD2Factory(nullptr)
	, mWriteFactory(nullptr)
	, mWICFactory(nullptr)
{
	HRESULT hr = S_OK;

	if (SUCCEEDED(hr))
	{
#if defined(DEBUG) || defined(_DEBUG)
		D2D1_FACTORY_OPTIONS opts{};
		opts.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

		hr = D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory),
			&opts,
			reinterpret_cast<void**>(&mD2Factory)
		);
#else
		hr = D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory),
			reinterpret_cast<void**>(&mD2Factory)
		);
#endif
	}
	if (SUCCEEDED(hr))
	{
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_ISOLATED,
			_uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&mWriteFactory)
		);
	}

	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			_uuidof(IWICImagingFactory),
			reinterpret_cast<void**>(&mWICFactory)
		);
	}
}

GraphicFactory::~GraphicFactory()
{
	LOG(L"GraphicFactory DTOR\n");
	if (mWriteFactory)
	{
		mWriteFactory->Release();
		mWriteFactory = nullptr;
	}
	
	if (mWICFactory)
	{
		mWICFactory->Release();
		mWICFactory = nullptr;
	}

	if (mD2Factory)
	{
		mD2Factory->Release();
		mD2Factory = nullptr;
	}
}

IDWriteFactory* GraphicFactory::GetWriteFactory()
{
	return mWriteFactory;
}

ID2D1Factory* GraphicFactory::GetD2Factory()
{
	return mD2Factory;
}

IWICImagingFactory* GraphicFactory::GetWICFactory()
{
	return mWICFactory;
}
