#pragma once
#include <Windows.h>

#include "Log.h"

class Com
{
public:
	enum Type
	{
		COINIT_APARTMENTTHREADED = 0x2,

#if  (_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) // DCOM
		// These constants are only valid on Windows NT 4.0
		COINIT_MULTITHREADED = COINITBASE_MULTITHREADED,
		COINIT_DISABLE_OLE1DDE = 0x4,      // Don't use DDE for Ole1 support.
		COINIT_SPEED_OVER_MEMORY = 0x8,      // Trade memory for speed.
#endif // DCOM


	};

	explicit Com(const DWORD type)
	{
		(void)CoInitializeEx(nullptr, type);

	}
	~Com()
	{
		LOG(L"COM DTOR\n");
		CoUninitialize();
	}

	Com(Com&) = delete;
	Com& operator=(Com&) = delete;

};