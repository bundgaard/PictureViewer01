#pragma once

extern "C"
{


	 void Log(const wchar_t* fmt, ...);

#ifdef _DEBUG
#define LOG(fmt, ...) Log(fmt, __VA_ARGS__)
#else 
#define LOG(fmt, ...)
#endif

}