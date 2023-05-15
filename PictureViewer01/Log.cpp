#include "Log.h"
#include <string>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
void Log(const wchar_t* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	std::wstring Text;
	Text.resize(_vscwprintf(fmt, args) + 1);

	_vsnwprintf_s(Text.data(), Text.size(), _TRUNCATE, fmt, args);
	OutputDebugStringW(Text.c_str());
	va_end(args);
};