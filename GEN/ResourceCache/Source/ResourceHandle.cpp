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
		std::cout << "Resource created: " << resCache->findPath(resource) << std::endl;
	}

	ResourceHandle::~ResourceHandle()
	{
		size_t memSize = buffer.size();
		buffer.clear();
		resCache->memoryHasBeenFreed(memSize, resource);
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
