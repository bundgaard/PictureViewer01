#pragma once
#include <string>
#include <memory>
#include <vector>

struct ZipFile;

class ZipManager
{
	int m_currentPage = -1;
	std::vector<std::unique_ptr<ZipFile>> m_zip_files;
	std::vector<HGLOBAL> m_zip_globals;
public:
	ZipManager();
	~ZipManager();
	void ReadZip(std::wstring const& filename);
	void Clear();
	size_t Size() const;
	std::unique_ptr<ZipFile>& Current();
	void Next();
	void Previous();
};

