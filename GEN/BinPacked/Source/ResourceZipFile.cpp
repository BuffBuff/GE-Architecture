#include "ResourceZipFile.h"

#include <fstream>

namespace GENA
{
	ResourceZipFile::ResourceZipFile(std::string filepath)
		:filepath(filepath)
	{
	}

	void ResourceZipFile::open()
	{
		pack.bindArchive(std::unique_ptr<std::istream>(new std::ifstream(filepath, std::ifstream::binary)));
	}

	uint64_t ResourceZipFile::getRawResourceSize(ResId res)
	{
		return pack.getFileSize(res);
	}

	void ResourceZipFile::getRawResource(ResId res, char* buffer)
	{
		pack.extractFile(res, buffer);
	}

	uint32_t ResourceZipFile::getNumResources() const
	{
		return pack.getNumFiles();
	}

	IResourceFile::ResId ResourceZipFile::getResourceId(uint32_t num) const
	{
		return pack.getFileId(num);
	}

	std::string ResourceZipFile::getResourceName(ResId res) const
	{
		return pack.getFileName(res);
	}

	std::string ResourceZipFile::getResourceType(ResId res) const
	{
		return pack.getFileType(res);
	}

}