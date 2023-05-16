#pragma once


template<typename T>
void SafeRelease(T*& t)
{
	if (nullptr != t)
	{
		t->Release();
		t = nullptr;
	}
}