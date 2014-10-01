#include "ResourceHandle.h"

#include "ResourceCache.h"

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
		resCache->memoryHasBeenFreed(buffer.size());
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
