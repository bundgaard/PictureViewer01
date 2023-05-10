#pragma once
#include <zip.h>
#include <string>

#include "Application.h"

class ZipFile
{
	std::string m_Name;
	size_t m_Size;
	void* m_GlobalData;
	HGLOBAL m_GlobalHandle;
	IStream* m_pStream;

public:
	ZipFile(std::string const& name, size_t size)
	{

	}

	~ZipFile()
	{

	}
};
class ZipReader
{
public:
	ZipReader() = default;
	~ZipReader();

	explicit ZipReader(std::string const& Filename);

};