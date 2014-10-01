#pragma once

#include "ResourceHandle.h"
#include "IResourceFile.h"
#include "IResourceLoader.h"

#include <list>
#include <map>
#include <memory>

namespace GENA
{
	typedef std::list<std::shared_ptr<ResourceHandle>> ResHandleList;
	typedef std::map<ResourceHandle::ResId, std::shared_ptr<ResourceHandle>> ResHandleMap;
	typedef std::list<std::shared_ptr<IResourceLoader>> ResourceLoaders;

	class ResourceCache
	{
		friend class ResourceHandle;

	public:
		typedef ResourceHandle::ResId ResId;

	protected:
		ResHandleList leastRecentlyUsed;
		ResHandleMap resources;
		ResourceLoaders resourceLoaders;

		std::unique_ptr<IResourceFile> file;

		uint64_t cacheSize;
		uint64_t allocated;

		std::shared_ptr<ResourceHandle> find(ResId res);
		void update(std::shared_ptr<ResourceHandle> handle);
		std::shared_ptr<ResourceHandle> load(ResId res);
		void free(std::shared_ptr<ResourceHandle> gonner);

		bool makeRoom(uint64_t size);
		char* allocate(uint64_t size);
		void freeOneResource();
		void memoryHasBeenFreed(uint64_t size);

	public:
		ResourceCache(uint64_t sizeInMiB, std::unique_ptr<IResourceFile>&& resFile);
		~ResourceCache();

		void init();
		void registerLoader(std::shared_ptr<IResourceLoader> loader);

		std::shared_ptr<ResourceHandle> getHandle(ResId res);
		int preload(const std::string pattern, void (*progressCallback)(int, bool&));
		void flush();
	};
}
