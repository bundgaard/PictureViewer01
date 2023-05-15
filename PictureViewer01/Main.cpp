#include "Application.h"
#include "Viewer.h"
#include "GraphicsManager.h"
#include "COM.h"
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")




int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpszCmdLine, _In_ int nCmdShow)
{



	
	COM cominit(COM::Type::COINIT_APARTMENTTHREADED | COM::Type::COINIT_DISABLE_OLE1DDE);
	GraphicsManager graphicManager;
	std::unique_ptr<Viewer> app;
	
	HRESULT hr = S_OK;
	if (SUCCEEDED(hr))
	{
		
		OutputDebugStringW(L"Initialized COM\n");
		app = std::make_unique<Viewer>(graphicManager);
		hr = app->Initialize(hInstance);

	}

	if (SUCCEEDED(hr))
	{
		app->Start();
	}
	return 0;
}