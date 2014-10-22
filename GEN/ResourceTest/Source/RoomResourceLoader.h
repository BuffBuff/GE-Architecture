#pragma once

#include <IResourceLoader.h>

struct RoomObject
{
	GENA::ResourceHandle::ResId id;
	float x, y, z;
};

class RoomResourceLoader : public GENA::IResourceLoader
{
public:
	std::string getPattern() override { return "room    "; }
	bool useRawFile() override { return false; }
	uint64_t getLoadedResourceSize(const GENA::Buffer& rawBuffer) override;
	bool loadResource(const GENA::Buffer& rawBuffer, std::shared_ptr<GENA::ResourceHandle> handle) override;
};