#pragma once
#include <vector>
#include <string>

class FolderManager
{
	int m_currentPage = 0;
	std::vector<std::wstring> m_files{};

public:
	void OpenFolder(std::wstring const& folderPath);
};

