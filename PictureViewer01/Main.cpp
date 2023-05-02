#include "Application.h"
#include "Viewer.h"

#pragma comment(lib, "d2d1")


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpszCmdLine, _In_ int nCmdShow)
{
	HRESULT hr = S_OK;
	if (SUCCEEDED(hr))
	{
		hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

		if (SUCCEEDED(hr))
		{
			OutputDebugStringW(L"Initialized COM\n");
			Viewer app;
			hr = app.Initialize(hInstance);
			if (SUCCEEDED(hr))
			{
				OutputDebugStringW(L"Initialized Viewer\n");
				MSG msg{};
				while (GetMessage(&msg, nullptr, 0, 0) != 0)
				{
					TranslateMessage(&msg);
					DispatchMessageW(&msg);
				}
			}
		}
		CoUninitialize();
	}

	return 0;
}