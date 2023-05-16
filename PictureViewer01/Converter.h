#pragma once
#include "Application.h"


inline std::wstring ToWideString(std::string const& text) //TODO: fix to separate CPP file
{
	const int textSize = static_cast<int>(text.size());
	auto const size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), textSize, nullptr, 0);
	std::wstring result;
	result.resize(size);

	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), textSize, result.data(), textSize);
	return result;
}
inline std::string FromWideString(std::wstring const& text) //TODO: fix to separate CPP file
{
	const int textSize = static_cast<int>(text.size());
	auto const size = WideCharToMultiByte(
		CP_UTF8,
		0,
		text.data(),
		textSize,
		nullptr,
		0,
		nullptr,
		nullptr);
	std::string result{};
	result.resize(size);
	WideCharToMultiByte(
		CP_UTF8,
		0,
		text.c_str(),
		textSize,
		result.data(),
		textSize,
		"",
		nullptr);
	return result;
}