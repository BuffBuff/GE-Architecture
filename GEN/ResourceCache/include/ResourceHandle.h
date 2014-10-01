#pragma once

#include <Buffer.h>

#include <cstdint>
#include <memory>

namespace GENA
{
	class ResourceHandle
	{
		friend class ResourceCache;

	public:
		typedef uint32_t ResId;

	protected:
		ResId resource;
		Buffer buffer;
		ResourceCache* resCache;

	public:
		ResourceHandle(ResId resId, Buffer&& buffer, ResourceCache* resCache);
		virtual ~ResourceHandle();

		Buffer& getBuffer();
		const Buffer& getBuffer() const;
	};
}
