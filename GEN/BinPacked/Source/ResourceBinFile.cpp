#include "ResourceBinFile.h"

#include <fstream>

namespace GENA
{
	ResourceBinFile::ResourceBinFile(std::string filepath)
		: filepath(filepath)
	{
	}

	void ResourceBinFile::open()
	{
		pack.bindArchive(std::unique_ptr<std::istream>(new std::ifstream(filepath, std::ifstream::binary)));
	}

	uint64_t ResourceBinFile::getRawResourceSize(ResId res)
	{
		return pack.getFileSize(res);
	}

	void ResourceBinFile::getRawResource(ResId res, char* buffer)
	{
		pack.extractFile(res, buffer);
	}

	uint32_t ResourceBinFile::getNumResources() const
	{
		return pack.getNumFiles();
	}

	IResourceFile::ResId ResourceBinFile::getResourceId(uint32_t num) const
	{
		return pack.getFileId(num);
	}

	std::string ResourceBinFile::getResourceName(ResId res) const
	{
		return pack.getFileName(res);
	}

	std::string ResourceBinFile::getResourceType(ResId res) const
	{
		return pack.getFileType(res);
	}
}
