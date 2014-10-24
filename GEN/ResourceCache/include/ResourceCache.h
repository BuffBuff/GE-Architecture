#pragma once

#include "ResourceHandle.h"
#include "IResourceFile.h"
#include "IResourceLoader.h"

#include <atomic>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace GENA
{
	typedef std::list<std::shared_ptr<ResourceHandle>> ResHandleList;
	typedef std::map<ResourceHandle::ResId, std::shared_ptr<ResourceHandle>> ResHandleMap;
	typedef std::map<ResourceHandle::ResId, std::weak_ptr<ResourceHandle>> WeakResMap;
	typedef std::list<std::shared_ptr<IResourceLoader>> ResourceLoaders;

	class ResourceCache
	{
		friend class ResourceHandle;

	public:
		typedef ResourceHandle::ResId ResId;

	protected:
		ResHandleList leastRecentlyUsed;
		std::recursive_mutex leastRecentlyUsedLock;
		ResHandleMap resources;
		WeakResMap weakResources;
		std::recursive_mutex resourcesLock;
		ResourceLoaders resourceLoaders;

		std::unique_ptr<IResourceFile> file;

		uint64_t cacheSize;
		uint64_t allocated;
		uint64_t maxAllocated;

		std::map<ResId, std::thread> workers;
		std::recursive_mutex workerLock;

		std::shared_ptr<ResourceHandle> find(ResId res);
		void update(std::shared_ptr<ResourceHandle> handle);
		std::shared_ptr<ResourceHandle> load(ResId res);
		void asyncLoad(ResId res, void (*completionCallback)(std::shared_ptr<ResourceHandle>, void*), void* userData);
		void free(std::shared_ptr<ResourceHandle> gonner);

		void makeRoom(uint64_t size);
		char* allocate(uint64_t size);
		void freeOneResource();
		void memoryHasBeenFreed(uint64_t size, ResId resId);

	public:
		ResourceCache(uint64_t sizeInMiB, std::unique_ptr<IResourceFile>&& resFile);
		~ResourceCache();

		void init();
		void registerLoader(std::shared_ptr<IResourceLoader> loader);

		std::shared_ptr<ResourceHandle> getHandle(ResId res);
		void preload(ResId res, void (*completionCallback)(std::shared_ptr<ResourceHandle>, void*), void* userData);
		void flush();

		ResId findByPath(const std::string path) const;
		std::string findPath(ResId res) const;

		uint64_t getMaxMemAllocated() const;
	};
}
