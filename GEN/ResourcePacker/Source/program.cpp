#include "Index.h"

#include <BinPacked.h>
#include <ZipPacked.h>

#include <fstream>
#include <iostream>

void addFile(const char* filename, const char* resourceType)
{
	GENA::Index index;

	{
		std::ifstream inFile("index.txt");
		if (inFile)
		{
			index.load(inFile);
		}
	}

	std::string name(filename);
	std::string type;

	if (resourceType == nullptr)
	{
		size_t lastDot = name.find_last_of('.');
		if (lastDot >= name.length())
		{
			type = "raw";
		}
		else
		{
			type = name.substr(lastDot + 1);
		}
	}
	else
	{
		type.assign(resourceType);
	}

	try
	{
		index.addEntry(name, type);
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	if (index.getHasChanged())
	{
		std::ofstream outFile("index.txt", std::ofstream::trunc);
		if (!outFile)
		{
			std::cerr << "Failed to open \"index.txt\" for writing.\n";
		}

		index.save(outFile);
	}
}

template <typename Pack>
void createResourceArchive(const char* filename)
{
	Pack pack;

	std::ifstream file("index.txt");
	if (!file)
	{
		std::cerr << "Failed to open \"index.txt\" for reading.\n";
	}

	GENA::Index index;
	index.load(file);

	index.exportToPack(pack);

	pack.write(std::ofstream(filename, std::ios::binary | std::ios::trunc));
}

template <>
void createResourceArchive<GENA::BinPacked>(const char* filename)
{
	GENA::BinPacked pack;

	std::ifstream file("index.txt");
	if (!file)
	{
		std::cerr << "Failed to open \"index.txt\" for reading.\n";
	}

	GENA::Index index;
	index.load(file);

	index.exportToPack(pack);

	pack.prepareFileInfo();
	pack.write(std::ofstream(filename, std::ios::binary | std::ios::trunc));
}

void removeFile(const char* filename)
{
	std::ifstream inFile("index.txt");
	if (!inFile)
	{
		std::cerr << "Failed to open \"index.txt\" for reading.\n";
		return;
	}

	GENA::Index index;
	index.load(inFile);

	try
	{
		index.removeEntry(filename);
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	if (index.getHasChanged())
	{
		std::ofstream outFile("index.txt", std::ofstream::trunc);
		if (!outFile)
		{
			std::cerr << "Failed to open \"index.txt\" for writing.\n";
		}

		index.save(outFile);
	}
}

template <typename Pack>
void extractFile(const char* resourceFilename, const char* resourceName, const char* destinationFilename)
{
	Pack pack;

	pack.bindArchive(std::unique_ptr<std::istream>(new std::ifstream(resourceFilename, std::ifstream::binary)));

	std::vector<char> data;

	unsigned int numFiles = pack.getNumFiles();
	for (unsigned int i = 0; i < numFiles; ++i)
	{
		Pack::ResId id = pack.getFileId(i);
		if (pack.getFileName(id) == resourceName)
		{
			data.resize((size_t)pack.getFileSize(id));
			pack.extractFile(id, data.data());
			break;
		}
	}

	std::ofstream(destinationFilename, std::ofstream::binary).write(data.data(), data.size());
}

int main(int argc, char * argv[])
try
{
	std::string usage = "Usage: ";
	usage.append(argv[0]).append(" <command> [args]\n"
		"\n"
		"Commands:\n"
		"    add <file> [resource type]\n"
		"            Add a file to the database of tracked resource files.\n"
		"    create <resource file type> <output file>\n"
		"            Create a resource archive from any currently tracked files\n"
		"            <resource file type> may be one of 'Zip' and 'Raw'\n"
		"    remove <file>\n"
		"            Remove a file from the database if tracked resource files\n"
		"    extract <resource file type> <pack file> <resource name> <destination file>\n"
		"            Extract a file from a resource pack\n");

	if (argc < 2)
	{
		std::cerr << usage;
		return EXIT_FAILURE;
	}

	if (_stricmp(argv[1], "add") == 0)
	{
		if (argc == 3)
		{
			addFile(argv[2], nullptr);
		}
		else if (argc == 4)
		{
			addFile(argv[2], argv[3]);
		}
		else
		{
			std::cerr << "Usage: " << argv[0] << " add <file> [resource type]\n";
			return EXIT_FAILURE;
		}
	}
	else if (_stricmp(argv[1], "create") == 0)
	{
		if (argc == 4)
		{
			if (_stricmp(argv[2], "Zip") == 0)
			{
				createResourceArchive<GENA::ZipPacked>(argv[3]);
			}
			else if (_stricmp(argv[2], "Raw") == 0)
			{
				createResourceArchive<GENA::BinPacked>(argv[3]);
			}
			else
			{
				std::cerr << "Unrecognized resource pack type: Only 'Zip' and 'Raw' supported\n";
				return EXIT_FAILURE;
			}
		}
		else
		{
			std::cerr << "Usage: " << argv[0] << " create <resource file type> <output file>\n";
			return EXIT_FAILURE;
		}
	}
	else if (_stricmp(argv[1], "remove") == 0)
	{
		if (argc == 3)
		{
			removeFile(argv[2]);
		}
		else
		{
			std::cerr << "Usage: " << argv[0] << " remove <file>\n";
			return EXIT_FAILURE;
		}
	}
	else if (_stricmp(argv[1], "extract") == 0)
	{
		if (argc == 6)
		{
			if (_stricmp(argv[2], "Zip") == 0)
			{
				extractFile<GENA::ZipPacked>(argv[3], argv[4], argv[5]);
			}
			else if (_stricmp(argv[2], "Raw") == 0)
			{
				extractFile<GENA::BinPacked>(argv[3], argv[4], argv[5]);
			}
			else
			{
				std::cerr << "Unrecognized resource pack type: Only 'Zip' and 'Raw' supported\n";
				return EXIT_FAILURE;
			}
		}
		else
		{
			std::cerr << "Usage: " << argv[0] << " extract <resource file type> <pack file> <resource name> <destination file>\n";
		}
	}
	else
	{
		std::cerr << usage;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
catch (std::exception& ex)
{
	std::cerr << ex.what() << std::endl;
#ifdef _DEBUG
	throw;
#endif
}
