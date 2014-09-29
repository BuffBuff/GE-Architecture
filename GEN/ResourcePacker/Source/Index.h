#pragma once

#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <string>

namespace GENA
{
	class Index
	{
	public:
		typedef uint32_t ResId;

	private:
		struct Entry
		{
			std::string resType;
			std::string filename;
		};

		std::map<ResId, Entry> entries;
		std::map<std::string, ResId> filenameToId;

		bool hasChanged;

		std::default_random_engine randGenerator;

	public:
		Index();
		void load(std::istream& indexFile);
		void save(std::ostream& indexFile) const;

		void addEntry(const std::string& filename, const std::string resType);
		void removeEntry(const std::string& filename);

		bool getHasChanged() const;

		template <typename Pack>
		void exportToPack(Pack& pack) const
		{
			for (const auto& entryPair : entries)
			{
				pack.addFile(entryPair.first, entryPair.second.filename, entryPair.second.resType);
			}
		}
	};
}
