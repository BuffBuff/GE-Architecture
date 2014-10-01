#pragma once

#include "ResourceHandle.h"

#include <Buffer.h>

#include <cstdint>
#include <memory>
#include <string>

namespace GENA
{
	class IResourceLoader
	{
	public:
		virtual std::string getPattern() = 0;
		virtual bool useRawFile() = 0;
		virtual uint64_t getLoadedResourceSize(const Buffer& rawBuffer) = 0;
		virtual bool loadResource(const Buffer& rawBuffer, std::shared_ptr<ResourceHandle> handle) = 0;
	};
}
