#include "ResourceCache.h"

#include "DefaultResourceLoader.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

namespace GENA
{
	std::shared_ptr<ResourceHandle> ResourceCache::find(ResId res)
	{
		std::unique_lock<std::recursive_mutex> lLock(leastRecentlyUsedLock, std::defer_lock);
		std::unique_lock<std::recursive_mutex> rLock(resourcesLock, std::defer_lock);
		std::lock(lLock, rLock);

		auto iter = resources.find(res);
		if (iter != resources.end())
		{
			return iter->second;
		}

		auto weakIter = weakResources.find(res);
		if (weakIter != weakResources.end())
		{
			std::shared_ptr<ResourceHandle> handle = weakIter->second.lock();
			weakResources.erase(res);

			if (handle)
			{
				resources[res] = handle;
				leastRecentlyUsed.push_front(handle);

				return handle;
			}
		}

		return std::shared_ptr<ResourceHandle>();
	}

	void ResourceCache::update(std::shared_ptr<ResourceHandle> handle)
	{
		std::lock_guard<std::recursive_mutex> lock(leastRecentlyUsedLock);

		leastRecentlyUsed.remove(handle);
		leastRecentlyUsed.push_front(handle);
	}

	std::shared_ptr<ResourceHandle> ResourceCache::load(ResId res)
	{
		std::shared_ptr<IResourceLoader> loader;
		std::shared_ptr<ResourceHandle> handle;

		for (auto testLoader : resourceLoaders)
		{
			if (testLoader->getPattern() == file->getResourceType(res))
			{
				loader = testLoader;
				break;
			}
		}

		if (!loader && !resourceLoaders.empty())
		{
			loader = resourceLoaders.back();
		}

		if (!loader)
		{
			throw std::runtime_error("Default resource loader not found!");
		}

		uint64_t rawSize = file->getRawResourceSize(res);
		char* rawCharBuffer = loader->useRawFile() ? allocate(rawSize) : new char[(size_t)rawSize];

		if (rawCharBuffer == nullptr)
		{
			// Out of cache memory
			return std::shared_ptr<ResourceHandle>();
		}

		Buffer rawBuffer(rawCharBuffer, (size_t)rawSize);

		file->getRawResource(res, rawBuffer.data());
		uint64_t size = 0;

		if (loader->useRawFile())
		{
			handle = std::shared_ptr<ResourceHandle>(new ResourceHandle(res, std::move(rawBuffer), this));
		}
		else
		{
			size = loader->getLoadedResourceSize(rawBuffer);
			Buffer buffer(allocate(size), (size_t)size);
			if (rawBuffer.data() == nullptr || buffer.data() == nullptr)
			{
				// Out of cache memory
				return std::shared_ptr<ResourceHandle>();
			}
			handle = std::shared_ptr<ResourceHandle>(new ResourceHandle(res, std::move(buffer), this));

			if (!loader->loadResource(rawBuffer, handle))
			{
				// Out of cache memory
				return std::shared_ptr<ResourceHandle>();
			}
		}

		if (handle)
		{
			std::unique_lock<std::recursive_mutex> lLock(leastRecentlyUsedLock, std::defer_lock);
			std::unique_lock<std::recursive_mutex> rLock(resourcesLock, std::defer_lock);
			std::lock(lLock, rLock);

			leastRecentlyUsed.push_front(handle);
			resources[res] = handle;
		}

		return handle;
	}

	void ResourceCache::asyncLoad(ResId res, void (*completionCallback)(std::shared_ptr<ResourceHandle>, void*), void* userData)
	{
		std::shared_ptr<ResourceHandle> handle = load(res);
		if (handle)
		{
			completionCallback(handle, userData);
		}
		
		std::thread me;
		{
			std::lock_guard<std::recursive_mutex> rLock(workerLock);
			me = std::move(workers[res]);
			workers.erase(res);
		}

		std::lock_guard<std::mutex> lock(doneWorkersLock);
		doneWorkers.push_back(std::move(me));
	}

	void ResourceCache::free(std::shared_ptr<ResourceHandle> gonner)
	{
		std::unique_lock<std::recursive_mutex> lLock(leastRecentlyUsedLock, std::defer_lock);
		std::unique_lock<std::recursive_mutex> rLock(resourcesLock, std::defer_lock);
		std::lock(lLock, rLock);

		resources.erase(gonner->resource);
		leastRecentlyUsed.remove(gonner);

		std::weak_ptr<ResourceHandle> weakGonner = gonner;
		gonner.reset();
		gonner = weakGonner.lock();

		if (gonner)
		{
			weakResources[gonner->resource] = gonner;
		}
	}

	void ResourceCache::makeRoom(uint64_t size)
	{
		if (size > cacheSize)
		{
			throw std::runtime_error("Object to large for cache");
		}

		while (size > cacheSize - allocated)
		{
			if (leastRecentlyUsed.empty())
			{
				std::streamsize oldWidth = std::cerr.width();
				std::streamsize idWidth = std::numeric_limits<ResId>::digits10 + 1;
				std::streamsize sizeWidth = std::numeric_limits<size_t>::digits10 + 1;

				std::cerr << "Failed to make room for resource\n";
				std::cerr << "\nResources currently loaded:\n";
				for (const auto& res : weakResources)
				{
					std::shared_ptr<ResourceHandle> handle = res.second.lock();
					if (handle)
					{
						std::cerr << std::setw(idWidth) << res.first << " "
							<< std::setw(sizeWidth) << handle->getBuffer().size() << std::setw(oldWidth) << " B "
							<< file->getResourceName(res.first) << "\n";
					}
				}
				std::cerr << std::endl;
				break;
			}

			freeOneResource();
		}
	}

	char* ResourceCache::allocate(uint64_t size)
	{
		std::unique_lock<std::recursive_mutex> lLock(leastRecentlyUsedLock, std::defer_lock);
		std::unique_lock<std::recursive_mutex> rLock(resourcesLock, std::defer_lock);
		std::lock(lLock, rLock);

		makeRoom(size);

		char* mem = new char[(size_t)size];
		if (mem)
		{
			uint64_t alloced = allocated += size;
			if (alloced > maxAllocated)
			{
				maxAllocated = alloced;
			}
		}

		return mem;
	}

	void ResourceCache::freeOneResource()
	{
		std::shared_ptr<ResourceHandle> handle = leastRecentlyUsed.back();
		leastRecentlyUsed.pop_back();

		resources.erase(handle->resource);
		leastRecentlyUsed.remove(handle);

		std::weak_ptr<ResourceHandle> weakGonner = handle;
		handle.reset();
		handle = weakGonner.lock();

		if (handle)
		{
			weakResources[handle->resource] = handle;
		}
	}

	void ResourceCache::memoryHasBeenFreed(uint64_t size, ResId resId)
	{
		std::unique_lock<std::recursive_mutex> lLock(leastRecentlyUsedLock, std::defer_lock);
		std::unique_lock<std::recursive_mutex> rLock(resourcesLock, std::defer_lock);
		std::lock(lLock, rLock);

		allocated -= size;
		if (weakResources.count(resId) > 0)
		{
			weakResources.erase(resId);
		}
	}

	ResourceCache::ResourceCache(uint64_t sizeInMiB, std::unique_ptr<IResourceFile>&& resFile)
		: cacheSize(sizeInMiB * 1024 * 1024),
		allocated(0),
		maxAllocated(0),
		file(std::move(resFile))
	{
	}

	ResourceCache::~ResourceCache()
	{
		for (auto& t : workers)
		{
			t.second.join();
		}
		workers.clear();

		for (auto& t : doneWorkers)
		{
			t.join();
		}
		doneWorkers.clear();

		while (!leastRecentlyUsed.empty())
		{
			freeOneResource();
		}
	}

	void ResourceCache::init()
	{
		file->open();
		registerLoader(std::shared_ptr<IResourceLoader>(new DefaultResourceLoader()));
	}

	void ResourceCache::registerLoader(std::shared_ptr<IResourceLoader> loader)
	{
		resourceLoaders.push_front(loader);
	}

	std::shared_ptr<ResourceHandle> ResourceCache::getHandle(ResId res)
	{
		std::unique_lock<std::recursive_mutex> lock(workerLock);

		std::shared_ptr<ResourceHandle> handle(find(res));
		if (!handle)
		{
			if (workers.count(res) == 0)
			{
				handle = load(res);
			}
			else
			{
				workers[res].join();
				workers.erase(res);
				handle = find(res);
			}
		}
		else
		{
			update(handle);
		}

		return handle;
	}

	void ResourceCache::preload(ResId res, void (*completionCallback)(std::shared_ptr<ResourceHandle>, void*), void* userData)
	{
		std::shared_ptr<ResourceHandle> handle;

		{
			std::unique_lock<std::recursive_mutex> lock(workerLock);

			handle = find(res);
			if (!handle)
			{
				if (workers.count(res) == 0)
				{
					std::thread newWorker(&ResourceCache::asyncLoad, this, res, completionCallback, userData);
					workers.emplace(res, std::move(newWorker));
				}

				return;
			}
		}

		completionCallback(handle, userData);

		std::lock_guard<std::mutex> lock(doneWorkersLock);
		for (auto& doneThread : doneWorkers)
		{
			doneThread.join();
		}
		doneWorkers.clear();
	}

	ResourceCache::ResId ResourceCache::findByPath(const std::string path) const
	{
		const uint32_t numRes = file->getNumResources();
		for (uint32_t i = 0; i < numRes; ++i)
		{
			ResId resId = file->getResourceId(i);
			if (file->getResourceName(resId) == path)
			{
				return resId;
			}
		}

		throw std::runtime_error(path + " could not be mapped to a resource");
	}

	std::string ResourceCache::findPath(ResId res) const
	{
		return file->getResourceName(res);
	}

	uint64_t ResourceCache::getMaxMemAllocated() const
	{
		return maxAllocated;
	}
}
