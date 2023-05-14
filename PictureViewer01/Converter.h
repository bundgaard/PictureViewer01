#pragma once
#include "Application.h"


std::wstring ToWideString(std::string const& Text)
{

	auto const Size = MultiByteToWideChar(CP_UTF8, 0, Text.c_str(), Text.size(), nullptr, 0);
	std::wstring Result;
	Result.resize(Size);
	MultiByteToWideChar(CP_UTF8, 0, Text.c_str(), Text.size(), Result.data(), Result.size());
	return Result;
}

std::string FromWideString(std::wstring const& Text)
{
	auto const Size = WideCharToMultiByte(CP_UTF8, 0, Text.data(), static_cast<int>(Text.size()), nullptr, 0, nullptr, nullptr);
	std::string Result{};
	Result.resize(Size);
	WideCharToMultiByte(
		CP_UTF8,
		0,
		Text.c_str(),
		Text.size(),
		Result.data(),
		Result.size(),
		"",
		nullptr);
	return Result;
}