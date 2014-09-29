#include "Index.h"

#include <chrono>

namespace GENA
{
	Index::Index()
		: hasChanged(false),
		randGenerator((std::default_random_engine::result_type)
			std::chrono::system_clock::now().time_since_epoch().count())
	{
	}

	void Index::load(std::istream& indexFile)
	{
		entries.clear();
		filenameToId.clear();
		hasChanged = false;

		std::string line;
		while(std::getline(indexFile, line))
		{
			size_t idN = line.find(' ');
			ResId id = std::stoul(line.substr(0, idN));

			std::string resType = line.substr(idN + 1, 8);
			std::string filename = line.substr(idN + 10);

			entries[id].resType = resType;
			entries[id].filename = filename;
			filenameToId[filename] = id;
		}
	}

	void Index::save(std::ostream& indexFile) const
	{
		for (const auto& entryPair : entries)
		{
			indexFile << entryPair.first << ' ' << entryPair.second.resType << ' ' << entryPair.second.filename << '\n';
		}
	}

	void Index::addEntry(const std::string& filename, const std::string resType)
	{
		if (resType.length() > 8)
		{
			throw std::runtime_error("Resource type may not be more than 8 characters long");
		}

		if (filenameToId.count(filename) != 0)
		{
			return;
		}

		std::string paddedResType = resType + std::string(8 - resType.length(), ' ');

		while (true)
		{
			ResId id = randGenerator();
			if (entries.count(id) == 0)
			{
				entries[id].filename = filename;
				entries[id].resType = paddedResType;

				filenameToId[filename] = id;
				break;
			}
		}

		hasChanged = true;
	}

	void Index::removeEntry(const std::string& filename)
	{
		if (filenameToId.count(filename) == 0)
		{
			throw std::runtime_error(filename + " not found in index");
		}

		ResId id = filenameToId[filename];
		filenameToId.erase(filename);
		entries.erase(id);

		hasChanged = true;
	}

	bool Index::getHasChanged() const
	{
		return hasChanged;
	}
}
