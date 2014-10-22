#include "ResourceHandle.h"

#include "ResourceCache.h"

#include <iostream>

namespace GENA
{
	ResourceHandle::ResourceHandle(ResId resId, Buffer&& buffer, ResourceCache* resCache)
		: resource(resId),
		buffer(std::move(buffer)),
		resCache(resCache)
	{
	}

	ResourceHandle::~ResourceHandle()
	{
		buffer.clear();
		resCache->memoryHasBeenFreed(buffer.size(), resource);
		std::cerr << "Resource released: " << resCache->findPath(resource) << std::endl;
	}

	Buffer& ResourceHandle::getBuffer()
	{
		return buffer;
	}

	const Buffer& ResourceHandle::getBuffer() const
	{
		return buffer;
	}
}
