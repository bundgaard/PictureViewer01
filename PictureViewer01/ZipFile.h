#pragma once
#include <vector>
#include <string>

#include <Windows.h>

#include "SafeRelease.h"

struct ZipMemory
{
	void* GlobalData;
	HGLOBAL Global;
};

struct ZipFile
{
	std::string Name;
	size_t Size;
	void* GlobalData;
	HGLOBAL Global;
	IStream* Stream;

	ZipFile(std::string const name, size_t size) :
		Name(name),
		Size(size),
		GlobalData(nullptr),
		Global(nullptr),
		Stream(nullptr)
	{

	}

	~ZipFile()
	{
		OutputDebugStringW(L"ZipFile destructor\n");
		if (Global)
			GlobalFree(Global);
		SafeRelease(Stream);
	}

	HRESULT Write(std::vector<byte>&& Bytes)
	{
		HRESULT hr = S_OK;
		if (SUCCEEDED(hr))
		{
			Global = GlobalAlloc(GMEM_MOVEABLE, Bytes.size());
			hr = Global ? S_OK : E_FAIL;
		}
		if (SUCCEEDED(hr))
		{
			GlobalData = GlobalLock(Global);
			hr = GlobalData ? S_OK : E_FAIL;
		}

		if (SUCCEEDED(hr))
		{
			CopyMemory(GlobalData, Bytes.data(), Bytes.size());
			hr = GlobalUnlock(GlobalData) ? S_OK : E_FAIL;
		}

		return hr;
	}

	HRESULT RecreateStream()
	{
		SafeRelease(Stream);
		return CreateStreamOnHGlobal(Global, false, &Stream);
	}
};

struct ZipList
{
	std::vector<ZipFile> m_list;
	std::vector<ZipMemory> m_memory_list;


};