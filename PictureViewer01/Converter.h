#pragma once
#include "Application.h"


std::wstring ToWideString(std::string const& Text)
{

	int TextSize = static_cast<int>(Text.size());
	auto const Size = MultiByteToWideChar(CP_UTF8, 0, Text.c_str(), TextSize, nullptr, 0);
	std::wstring Result;
	Result.resize(Size);
	
	MultiByteToWideChar(CP_UTF8, 0, Text.c_str(), TextSize, Result.data(), TextSize);
	return Result;
}

std::string FromWideString(std::wstring const& Text)
{
	int TextSize = static_cast<int>(Text.size());
	auto const Size = WideCharToMultiByte(CP_UTF8, 0, Text.data(), TextSize, nullptr, 0, nullptr, nullptr);
	std::string Result{};
	Result.resize(Size);
	WideCharToMultiByte(
		CP_UTF8,
		0,
		Text.c_str(),
		TextSize,
		Result.data(),
		TextSize,
		"",
		nullptr);
	return Result;
}