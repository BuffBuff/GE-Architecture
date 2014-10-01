#pragma once

#include <IResourceLoader.h>

namespace GENA
{
	class DefaultResourceLoader : public IResourceLoader
	{
	public:
		std::string getPattern() override { return "png     "; }
		bool useRawFile() override { return true; }
		uint64_t getLoadedResourceSize(const Buffer& rawBuffer) override { return rawBuffer.size(); }
		bool loadResource(const Buffer& rawBuffer, std::shared_ptr<ResourceHandle> handle) override { return true; }
	};
}
