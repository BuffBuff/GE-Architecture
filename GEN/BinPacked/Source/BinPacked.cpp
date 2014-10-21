#include "BinPacked.h"

#include <fstream>

namespace GENA
{
	void BinPacked::Index::addEntry(ResId resId, const std::string& filename, const std::string resType)
	{
		entries[resId].filename = filename;
		entries[resId].resType = resType;
		resourceIds.push_back(resId);
	}

	void BinPacked::Index::addEntry(ResId resId, const Entry& entry)
	{
		entries[resId] = entry;
		resourceIds.push_back(resId);
	}

	const BinPacked::Entry& BinPacked::Index::getEntry(ResId id) const
	{
		return entries.at(id);
	}

	BinPacked::ResId BinPacked::Index::getResAt(uint32_t num) const
	{
		return resourceIds[num];
	}

	const std::map<BinPacked::ResId, BinPacked::Entry>& BinPacked::Index::getEntries() const
	{
		return entries;
	}

	void BinPacked::Index::prepareFileInfo()
	{
		uint64_t currPos = calcHeaderSize();
		for (auto& entryPair : entries)
		{
			currPos = updateFileInfo(entryPair.second, currPos);
		}
	}

	uint64_t BinPacked::Index::calcHeaderSize()
	{
		uint64_t res = sizeof(uint32_t);
		for (const auto& entryPair : entries)
		{
			res += sizeof(ResId) + sizeof(uint64_t) * 2 + 8
				+ sizeof(uint16_t) + entryPair.second.filename.length();
		}
		return res;
	}

	uint64_t BinPacked::Index::updateFileInfo(Entry& fileEntry, uint64_t currPos)
	{
		std::ifstream file(fileEntry.filename);
		if (!file)
		{
			throw std::runtime_error(fileEntry.filename + " could not be opened");
		}

		file.seekg(0, std::ios_base::end);
		auto fileSize = file.tellg();

		if (fileSize < 0 || (uint64_t)fileSize > std::numeric_limits<uint64_t>::max())
		{
			throw std::runtime_error("Invalid file size (" + std::to_string(fileSize) + ")");
		}

		fileEntry.fileSize = fileSize;
		fileEntry.filepos = currPos;
		return currPos + fileSize;
	}

	void BinPacked::bindArchive(std::unique_ptr<std::istream> archive)
	{
		this->archive.swap(archive);

		readIndex();
	}

	void BinPacked::prepareFileInfo()
	{
		index.prepareFileInfo();
	}

	void BinPacked::write(std::ostream& out) const
	{
		const std::map<ResId, Entry>& entries = index.getEntries();

		ser(out, (uint32_t)entries.size());

		for (const auto& entryPair : entries)
		{
			ser(out, entryPair);
		}

		for (const auto& entryPair : entries)
		{
			writeFile(out, entryPair.second);
		}
	}

	void BinPacked::addFile(ResId id, const std::string& filename, const std::string resType)
	{
		index.addEntry(id, filename, resType);
	}

	uint64_t BinPacked::getFileSize(ResId id) const
	{
		return index.getEntry(id).fileSize;
	}

	void BinPacked::extractFile(ResId id, char* buffer) const
	{
		const Entry& entry = index.getEntry(id);

		std::lock_guard<std::mutex> lock(archiveLock);
		archive->seekg(entry.filepos);
		archive->read(buffer, entry.fileSize);
	}

	uint32_t BinPacked::getNumFiles() const
	{
		return index.getEntries().size();
	}

	BinPacked::ResId BinPacked::getFileId(uint32_t num) const
	{
		return index.getResAt(num);
	}

	std::string BinPacked::getFileName(ResId res) const
	{
		return index.getEntry(res).filename;
	}

	std::string BinPacked::getFileType(ResId res) const
	{
		return index.getEntry(res).resType;
	}

	void BinPacked::writeFile(std::ostream& out, const Entry& fileEntry)
	{
		auto startPos = out.tellp();
		if ((uint64_t)startPos != fileEntry.filepos)
		{
			throw std::runtime_error("File at wrong position for writing");
		}

		std::ifstream source(fileEntry.filename, std::ios::binary);
		if (!source)
		{
			throw std::runtime_error("Missing file: " + fileEntry.filename);
		}

		out << source.rdbuf();
		auto endPos = out.tellp();

		if (endPos - startPos != fileEntry.fileSize)
		{
			throw std::runtime_error("Wrote incorrect amount of data for file");
		}
	}

	void BinPacked::readIndex()
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
