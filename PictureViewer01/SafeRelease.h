#pragma once


template<typename T>
inline void SafeRelease(T*& t)
{
	if (NULL != t)
	{
		t->Release();
		t = nullptr;
	}
}