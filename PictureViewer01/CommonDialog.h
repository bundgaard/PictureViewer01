#pragma once
#include <Windows.h>

// DISCLAIMER: Would be nice to implement the new style of openfile, but for now I will just wait...


struct IFileDialog;
struct IFileOpenDialog;
class CommonDialog
{
	IFileDialog* mFileDialog;
public:
	CommonDialog(CommonDialog&) = delete;
	CommonDialog operator=(CommonDialog&) = delete;

	CommonDialog();
	~CommonDialog();
	HRESULT Initialize();
};

