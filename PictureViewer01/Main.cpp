#include "Application.h"
#include "Viewer.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpszCmdLine, _In_ int nCmdShow)
{



	HRESULT hr = S_OK;
	Viewer app;
	hr = app.Initialize(hInstance);
	if (SUCCEEDED(hr))
	{
		app.Start();
	}

	return 0;
}