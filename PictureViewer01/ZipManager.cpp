
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "ZipManager.h"
#include "ZipFile.h"
#include "Converter.h"

#include <zip.h>
#include <algorithm>
#include <memory>
#include "Log.h"
namespace
{
	inline void RemoveDuplicates(std::vector<std::unique_ptr<ZipFile>>& list)
	{
		auto Compare = [&](std::unique_ptr<ZipFile>& A, std::unique_ptr<ZipFile>& B) {
			return A->Name < B->Name;
		};

		std::sort(list.begin(), list.end(), Compare);
		auto EraseComparator = [](std::unique_ptr<ZipFile>& A, std::unique_ptr<ZipFile>& B) {
			return A->Name == B->Name;
		};
		auto it = std::unique(list.begin(), list.end(), EraseComparator);

		list.erase(it, list.end());
		LOG(L"End of Remove Duplicates\n");
	}
}
ZipManager::ZipManager() : m_currentPage(0)
{
}

ZipManager::~ZipManager()
{
	LOG(L"ZipManager DTOR\n");
}

void ZipManager::ReadZip(std::wstring const& Filename)
{
	Clear();
	zip* archive = zip_open(FromWideString(Filename).c_str(), 0, nullptr);
	if (!archive)
	{
		Log(L"Failed to open zip archive\n");
		return;
	}

	auto NumFiles = zip_get_num_files(archive);
	m_zip_files.reserve(NumFiles);

	for (int i = 0; i < NumFiles; i++)
	{
		struct zip_stat stat;
		zip_stat_index(archive, i, 0, &stat);

		zip_file* File = zip_fopen_index(archive, i, 0);
		if (File)
		{
			std::unique_ptr<ZipFile> ptr = std::make_unique<ZipFile>(stat.name, static_cast<size_t>(stat.size));
			std::vector<byte> Bytes;
			Bytes.resize(ptr->Size);
			zip_int64_t bytes_read = zip_fread(File, Bytes.data(), Bytes.size()); // TODO: could actually have a list and loop through it before hand or just do the work and do it after? I choose the latter.
			HRESULT hr = S_OK;
			hr = bytes_read == ptr->Size ? S_OK : E_FAIL;
			if (SUCCEEDED(hr))
			{
				hr = ptr->Write(std::move(Bytes));
			}

			if (SUCCEEDED(hr))
			{
				m_zip_files.push_back(std::move(ptr));
				zip_fclose(File);
			}
		}
	}
	zip_close(archive);
	auto Count = m_zip_files.size();
	LOG(L"Loaded %d files\n", Count);
	// Clean the zip list
	RemoveDuplicates(m_zip_files);
	LOG(L"Erased %d\n", Count - m_zip_files.size());
}

void ZipManager::Clear()
{
	m_zip_files.clear();
	m_currentPage = 0;
}

size_t ZipManager::Size()
{
	return m_zip_files.size();
}

std::unique_ptr<ZipFile>& ZipManager::Current()
{
	return m_zip_files.at(m_currentPage);
}

void ZipManager::Next()
{
	m_currentPage++;
	if (m_currentPage >= static_cast<int>(Size()))
	{
		m_currentPage = 0;
	}
}

void ZipManager::Previous()
{
	--m_currentPage;
	if (m_currentPage < 0)
	{
		m_currentPage = static_cast<int>(Size()) - 1;
	}
}
