#include "ResourceCache.h"

#include "DefaultResourceLoader.h"

namespace GENA
{
	std::shared_ptr<ResourceHandle> ResourceCache::find(ResId res)
	{
		std::lock_guard<std::mutex> lock(resourcesLock);

		auto iter = resources.find(res);
		if (iter == resources.end())
		{
			return std::shared_ptr<ResourceHandle>();
		}
		else
		{
			return iter->second;
		}
	}

	void ResourceCache::update(std::shared_ptr<ResourceHandle> handle)
	{
		std::lock_guard<std::mutex> lock(leastRecentlyUsedLock);

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
			std::unique_lock<std::mutex> lLock(leastRecentlyUsedLock, std::defer_lock);
			std::unique_lock<std::mutex> rLock(resourcesLock, std::defer_lock);
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
	}

	void ResourceCache::free(std::shared_ptr<ResourceHandle> gonner)
	{
		std::unique_lock<std::mutex> lLock(leastRecentlyUsedLock, std::defer_lock);
		std::unique_lock<std::mutex> rLock(resourcesLock, std::defer_lock);
		std::lock(lLock, rLock);

		resources.erase(gonner->resource);
		leastRecentlyUsed.remove(gonner);
	}

	bool ResourceCache::makeRoom(uint64_t size)
	{
		if (size > cacheSize)
		{
			return false;
		}

		while (size > cacheSize - allocated)
		{
			if (leastRecentlyUsed.empty())
			{
				return false;
			}

			freeOneResource();
		}

		return true;
	}

	char* ResourceCache::allocate(uint64_t size)
	{
		std::unique_lock<std::mutex> lLock(leastRecentlyUsedLock, std::defer_lock);
		std::unique_lock<std::mutex> rLock(resourcesLock, std::defer_lock);
		std::lock(lLock, rLock);

		if (!makeRoom(size))
		{
			return nullptr;
		}

		char* mem = new char[(size_t)size];
		if (mem)
		{
			allocated += size;
		}

		return mem;
	}

	void ResourceCache::freeOneResource()
	{
		std::shared_ptr<ResourceHandle> handle = leastRecentlyUsed.back();
		leastRecentlyUsed.pop_back();

		resources.erase(handle->resource);
	}

	void ResourceCache::memoryHasBeenFreed(uint64_t size)
	{
		allocated -= size;
	}

	ResourceCache::ResourceCache(uint64_t sizeInMiB, std::unique_ptr<IResourceFile>&& resFile)
		: cacheSize(sizeInMiB * 1024 * 1024),
		allocated(0),
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
		std::unique_lock<std::mutex> lock(workerLock);

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
		std::unique_lock<std::mutex> lock(workerLock);

		std::shared_ptr<ResourceHandle> handle(find(res));
		if (handle)
		{
			completionCallback(handle, userData);
		}
		else
		{
			if (workers.count(res) == 0)
			{
				std::thread newWorker(&ResourceCache::asyncLoad, this, res, completionCallback, userData);
				workers.emplace(res, std::move(newWorker));
			}
		}
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
}
