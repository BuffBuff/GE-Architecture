#include "ZipPacked.h"
#include <zlib.h>
#if defined( NDEBUG ) || ! defined( _DEBUG )
#pragma comment(lib, "zlibstatic")
#else
#pragma comment(lib, "zlibstaticd")
#endif

#include <fstream>

namespace GENA
{
	void ZipPacked::Index::addEntry(ResId resId, const std::string& filename, const std::string resType)
	{
		entries[resId].filename = filename;
		entries[resId].resType = resType;
		resourceIds.push_back(resId);
	}
	
	void ZipPacked::Index::addEntry(ResId resId, const Entry& entry)
	{
		entries[resId] = entry;
		resourceIds.push_back(resId);
	}

	const ZipPacked::Entry& ZipPacked::Index::getEntry(ResId id) const
	{
		return entries.at(id);
	}

	ZipPacked::ResId ZipPacked::Index::getResAt(uint32_t num) const
	{
		return resourceIds[num];
	}

	std::map<ZipPacked::ResId, ZipPacked::Entry>& ZipPacked::Index::getEntries()
	{
		return entries;
	}

	const std::map<ZipPacked::ResId, ZipPacked::Entry>& ZipPacked::Index::getEntries() const
	{
		return entries;
	}

	uint64_t ZipPacked::Index::calcHeaderSize()
	{
		uint64_t res = sizeof(uint32_t);
		for(const auto& entryPair : entries)
		{
			res += sizeof(ResId) + sizeof(uint64_t) * 3 + 8
				+ sizeof(uint16_t) + entryPair.second.filename.length();
		}
		return res;
	}

	void ZipPacked::bindArchive(std::unique_ptr<std::istream> archive)
	{
		this->archive.swap(archive);

		readIndex();
	}

	void ZipPacked::write(std::ostream& out)
	{
		std::map<ResId, Entry>& entries = index.getEntries();

		uint64_t headerSize = index.calcHeaderSize();
		out.seekp(headerSize);

		for (auto& entryPair : entries)
		{
			writeFile(out, entryPair.second);
		}

		out.seekp(0);

		ser(out, (uint32_t)entries.size());

		for (const auto& entryPair : entries)
		{
			ser(out, entryPair);
		}

		if((uint64_t)out.tellp() != headerSize)
		{
			throw std::runtime_error("Invalid HeaderSize");
		}

	}

	void ZipPacked::addFile(ResId id, const std::string& filename, const std::string resType)
	{
		index.addEntry(id, filename, resType);
	}

	uint64_t ZipPacked::getFileSize(ResId id) const
	{
		return index.getEntry(id).fileSize;
	}

	void ZipPacked::extractFile(ResId id, char* buffer) const
	{
		const Entry& entry = index.getEntry(id);
		uLongf destLen = (uLongf)entry.fileSize;
		Bytef *src = (Bytef*)malloc((size_t)entry.compSize);
		std::lock_guard<std::mutex> lock(archiveLock);
		archive->seekg(entry.filepos);
		archive->read((char*)src, entry.compSize);
		uncompress((Bytef *)buffer, &destLen, src, (uLongf)entry.compSize);

		free(src);
	}

	uint32_t ZipPacked::getNumFiles() const
	{
		return index.getEntries().size();
	}

	ZipPacked::ResId ZipPacked::getFileId(uint32_t num) const
	{
		return index.getResAt(num);
	}

	std::string ZipPacked::getFileName(ResId res) const
	{
		return index.getEntry(res).filename;
	}

	std::string ZipPacked::getFileType(ResId res) const
	{
		return index.getEntry(res).resType;
	}

	void ZipPacked::writeFile(std::ostream& out, Entry& fileEntry)
	{
		fileEntry.filepos = out.tellp();
		
		std::ifstream source(fileEntry.filename, std::ios::binary);
		source.seekg(0, std::ios_base::end);
		std::streampos length = source.tellg(); 
		source.seekg(0);
		uLongf destLen = (uLongf)compressBound((uLongf)length);
		Bytef *dest = (Bytef*)malloc((size_t)destLen);
		Bytef *src = (Bytef*)malloc((size_t)length);
		source.read((char*)src, length);
		compress(dest, &destLen, src, (uLongf)length);
		out.write((char*)dest, destLen);
		fileEntry.fileSize = length;
		fileEntry.compSize = destLen;

		free(dest);
		free(src);
	}

	void ZipPacked::readIndex()
	{
		uint32_t numEntries;
		des(*archive.get(), numEntries);

		for (unsigned int i = 0; i < numEntries; ++i)
		{
			std::pair<ResId, Entry> entry;
			des(*archive.get(), entry);
			index.addEntry(entry.first, entry.second);
		}
	}
}