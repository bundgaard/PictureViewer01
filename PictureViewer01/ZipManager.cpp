
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
	void RemoveDuplicates(std::vector<std::unique_ptr<ZipFile>>& list)
	{
		auto compare = [&](const std::unique_ptr<ZipFile>& A, const std::unique_ptr<ZipFile>& B) {
			return A->Name < B->Name;
		};

		std::ranges::sort(list, compare);
		auto eraseComparator = [](const std::unique_ptr<ZipFile>& A, const std::unique_ptr<ZipFile>& B) {
			return A->Name == B->Name;
		};


		list.erase(std::ranges::unique(list, eraseComparator).begin(), list.end());
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

void ZipManager::ReadZip(std::wstring const& filename)
{
	Clear();
	zip* archive = zip_open(FromWideString(filename).c_str(), 0, nullptr);
	if (!archive)
	{
		Log(L"Failed to open zip archive\n");
		return;
	}

	const auto numFiles = zip_get_num_files(archive);
	m_zip_files.reserve(numFiles);

	for (int i = 0; i < numFiles; i++)
	{
		struct zip_stat stat {};
		zip_stat_index(archive, i, 0, &stat);
		zip_file* File = zip_fopen_index(archive, i, 0);
		if (!File)
		{
			LOG(L"zip could not open index\n");
			continue;
		}

		auto ptr = std::make_unique<ZipFile>(stat.name, static_cast<size_t>(stat.size));
		std::vector<byte> bytes;
		bytes.resize(ptr->Size);
		const zip_int64_t bytesRead = zip_fread(File, bytes.data(), bytes.size()); // TODO: could actually have a list and loop through it before hand or just do the work and do it after? I choose the latter.

		HRESULT hr = S_OK;
		hr = static_cast<size_t>(bytesRead) == ptr->Size ? S_OK : E_FAIL;
		if (SUCCEEDED(hr))
		{
			hr = ptr->Write(std::move(bytes));
		}

		if (SUCCEEDED(hr))
		{
			m_zip_files.push_back(std::move(ptr));
			zip_fclose(File);
		}
	}
	zip_close(archive);
	const auto count = m_zip_files.size();
	LOG(L"Loaded %d files\n", count);
	// Clean the zip list
	RemoveDuplicates(m_zip_files);
	LOG(L"Erased %d\n", count - m_zip_files.size());
}

void ZipManager::Clear()
{
	m_zip_files.clear();
	m_currentPage = 0;
}

size_t ZipManager::Size() const
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

int ZipManager::CurrentPage() const
{
	return m_currentPage;
}
