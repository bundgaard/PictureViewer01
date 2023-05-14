#include "CommonDialog.h"
#include <ShlObj_core.h>
// DISCLAIMER: Would be nice to implement the new style of openfile, but for now I will just wait...

CommonDialog::CommonDialog() : mFileDialog(nullptr)
{

}

CommonDialog::~CommonDialog()
{
}

HRESULT CommonDialog::Initialize()
{
	HRESULT hr = S_OK;

	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&mFileDialog));
	}
	if (SUCCEEDED(hr))
	{
		IFileDialogEvents* dialogEvent = nullptr;
	}
	return hr;
}
