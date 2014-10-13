#pragma once

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace GENA
{
	class BinPacked
	{
	public:
			typedef uint32_t ResId;

	private:
		struct Entry
		{
			uint64_t filepos;
			uint64_t fileSize;
			std::string resType;
			std::string filename;
		};

		class Index
		{
		private:
			std::vector<ResId> resourceIds;
			std::map<ResId, Entry> entries;

		public:
			void addEntry(ResId resId, const std::string& filename, const std::string resType);
			void addEntry(ResId resId, const Entry& entry);
			const Entry& getEntry(ResId id) const;
			ResId getResAt(uint32_t num) const;
			const std::map<ResId, Entry>& getEntries() const;

			void prepareFileInfo();
			uint64_t calcHeaderSize();

		private:
			static uint64_t updateFileInfo(Entry& fileEntry, uint64_t currPos);
		};

		Index index;
		std::unique_ptr<std::istream> archive;
		mutable std::mutex archiveLock;

	public:
		void bindArchive(std::unique_ptr<std::istream> archive);

		void prepareFileInfo();
		void write(std::ostream& out) const;

		void addFile(ResId id, const std::string& filename, const std::string resType);
		uint64_t getFileSize(ResId id) const;
		void extractFile(ResId id, char* buffer) const;
		uint32_t getNumFiles() const;
		ResId getFileId(uint32_t num) const;
		std::string getFileName(ResId res) const;
		std::string getFileType(ResId res) const;

	private:
		static void writeFile(std::ostream& out, const Entry& fileEntry);

		void readIndex();

		template <typename ValType>
		static void ser(std::ostream& out, ValType val);

		template <>
		static void ser<uint16_t>(std::ostream& out, uint16_t val)
		{
			out.write((const char*)&val, sizeof(val));
		}

		template <>
		static void ser<uint32_t>(std::ostream& out, uint32_t val)
		{
			out.write((const char*)&val, sizeof(val));
		}

		template <>
		static void ser<uint64_t>(std::ostream& out, uint64_t val)
		{
			out.write((const char*)&val, sizeof(val));
		}

		template <typename First, typename Second>
		static void ser(std::ostream& out, const std::pair<First, Second>& val)
		{
			ser(out, val.first);
			ser(out, val.second);
		}

		template <>
		static void ser<Entry>(std::ostream& out, Entry val)
		{
			ser(out, val.filepos);
			ser(out, val.fileSize);
			out.write(val.resType.data(), 8);
			ser(out, val.filename);
		}

		template <>
		static void ser<std::string>(std::ostream& out, std::string val)
		{
			ser(out, (uint16_t)val.length());
			out.write(val.data(), val.length());
		}

		template <typename ValType>
		static void des(std::istream& in, ValType& val);

		template <>
		static void des<uint16_t>(std::istream& in, uint16_t& val)
		{
			in.read((char*)&val, sizeof(val));
		}

		template <>
		static void des<uint32_t>(std::istream& in, uint32_t& val)
		{
			in.read((char*)&val, sizeof(val));
		}

		template <>
		static void des<uint64_t>(std::istream& in, uint64_t& val)
		{
			in.read((char*)&val, sizeof(val));
		}

		template <typename First, typename Second>
		static void des(std::istream& in, std::pair<First, Second>& val)
		{
			des(in, val.first);
			des(in, val.second);
		}

		template <>
		static void des<Entry>(std::istream& in, Entry& val)
		{
			des(in, val.filepos);
			des(in, val.fileSize);

			char typeBuf[8];
			in.read(typeBuf, 8);
			val.resType.assign(typeBuf, 8);

			des(in, val.filename);
		}

		template <>
		static void des<std::string>(std::istream& in, std::string& val)
		{
			uint16_t length;
			des(in, length);

			std::vector<unsigned char> buffer(length);
			in.read((char*)buffer.data(), length);

			val.assign((char*)buffer.data(), length);
		}
	};
}
