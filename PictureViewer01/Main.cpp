#include "Application.h"
#include "Viewer.h"
#include "GraphicsManager.h"
#include "ZipManager.h"
#include "COM.h"
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")




int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpszCmdLine, _In_ int nCmdShow)
{



	
	Com cominit(Com::Type::COINIT_APARTMENTTHREADED | Com::Type::COINIT_DISABLE_OLE1DDE);
	ZipManager zipManager;
	GraphicsManager graphicManager;
	std::unique_ptr<Viewer> app;
	
	HRESULT hr = S_OK;
	if (SUCCEEDED(hr))
	{
		
		LOG(L"Initialized COM\n");
		app = std::make_unique<Viewer>(graphicManager, zipManager);
		hr = app->Initialize(hInstance);
	}

	if (SUCCEEDED(hr))
	{
		app->Start();
	}
	return 0;
}