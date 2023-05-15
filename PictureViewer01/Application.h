#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d2d1.h>
#include <wincodec.h>

#include <string>
#include <vector>


static void Log(const wchar_t* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	std::wstring Text;
	Text.resize(_vscwprintf(fmt, args) + 1);

	_vsnwprintf_s(Text.data(), Text.size(), _TRUNCATE, fmt, args);
	OutputDebugStringW(Text.c_str());
	va_end(args);
}; 
#ifdef _DEBUG
#define LOG(fmt, ...) Log(fmt, __VA_ARGS__)
#else 
#define LOG(fmt, ...)
#endif