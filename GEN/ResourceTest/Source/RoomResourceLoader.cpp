#include "RoomResourceLoader.h"

#include <sstream>

typedef GENA::ResourceHandle::ResId ResId;

uint64_t RoomResourceLoader::getLoadedResourceSize(const GENA::Buffer& rawBuffer)
{
	std::istringstream iss(std::string(rawBuffer.data(), rawBuffer.size()));
	std::string word;

	uint32_t numObjs = 0;

	iss >> word;
	while (iss)
	{
		if (word != "-")
		{
			return 0;
		}
		iss >> word;
		if (word != "res:")
		{
			return 0;
		}
		ResId res = 0;
		iss >> res;
		iss >> word;
		if (word != "x:")
		{
			return 0;
		}
		float x, y, z;
		iss >> x;
		iss >> word;
		if (word != "y:")
		{
			return 0;
		}
		iss >> y;
		iss >> word;
		if (word != "z:")
		{
			return 0;
		}
		iss >> z;
		iss >> word;

		++numObjs;
	}

	return sizeof(uint32_t) + sizeof(RoomObject) * numObjs;
}

bool RoomResourceLoader::loadResource(const GENA::Buffer& rawBuffer, std::shared_ptr<GENA::ResourceHandle> handle)
{
	std::istringstream iss(std::string(rawBuffer.data(), rawBuffer.size()));
	std::string word;

	uint32_t numObjs = 0;

	char* writePos = handle->getBuffer().data() + sizeof(uint32_t);
	RoomObject* roomWritePos = (RoomObject*)writePos;

	iss >> word;
	while (iss)
	{
		if (word != "-")
		{
			return false;
		}
		iss >> word;
		if (word != "res:")
		{
			return false;
		}
		iss >> roomWritePos->id;
		iss >> word;
		if (word != "x:")
		{
			return false;
		}
		iss >> roomWritePos->x;
		iss >> word;
		if (word != "y:")
		{
			return false;
		}
		iss >> roomWritePos->y;
		iss >> word;
		if (word != "z:")
		{
			return false;
		}
		iss >> roomWritePos->z;
		iss >> word;

		++roomWritePos;
		++numObjs;
	}

	*(uint32_t*)(handle->getBuffer().data()) = numObjs;

	return true;
}
