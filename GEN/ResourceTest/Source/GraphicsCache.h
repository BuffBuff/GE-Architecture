#pragma once

#include "GraphicsHandle.h"
#include "PoolAllocator.h"

#include <ResourceCache.h>
#include <ResourceHandle.h>

#include <atomic>
#include <functional>
#include <forward_list>
#include <map>
#include <memory>
#include <string>
#include <vector>

extern bool g_CText;

class IGraphics;

typedef std::function<void(std::shared_ptr<GraphicsHandle>)> GCreatedHandler;
typedef std::forward_list<GCreatedHandler> GCreatedHandlers;
typedef std::function<void(std::string)> GRemovedHandler;
typedef std::forward_list<GRemovedHandler> GRemovedHandlers;

typedef std::map<std::string, GCreatedHandlers> ResLoadingMap;

struct ModelReq
{
	std::string modelId;
	std::shared_ptr<GENA::ResourceHandle> resource;
	std::atomic_int texturesToLoad;
	std::vector<std::shared_ptr<GraphicsHandle>> children;

	ModelReq(std::string modelId, std::shared_ptr<GENA::ResourceHandle> resource)
		: modelId(modelId),
		resource(resource)
	{
		texturesToLoad = 0;
	}
};

typedef std::shared_ptr<ModelReq> ModelReqP;

struct TextureReq
{
	std::string textureId;
	std::shared_ptr<GENA::ResourceHandle> resource;
};

struct ModelRem
{
	std::string modelId;
	GRemovedHandler completionHandler;
};

struct TextureRem
{
	std::string textureId;
	GRemovedHandler completionHandler;
};
	
typedef std::function<void(std::shared_ptr<GENA::ResourceHandle>)> CompletionHandler;
typedef GENA::PoolAllocator<sizeof(CompletionHandler)> COMAlloc;

class GraphicsCache
{
public:
	typedef GENA::ResourceHandle::ResId ResId;

private:
	friend class GraphicsHandle;

	typedef std::map<std::string, std::weak_ptr<GraphicsHandle>> GraphicsModelResMap;
	typedef std::map<std::string, std::weak_ptr<GraphicsHandle>> GraphicsTextureResMap;
	typedef GENA::PoolAllocator<sizeof(GraphicsHandle)> GRHAlloc;

	template <typename Alloc, typename Obj>
	struct AllocDeleter
	{
		Alloc& alloc;

		AllocDeleter(Alloc& alloc)
			: alloc(alloc)
		{
		}

		void operator()(Obj* ptr)
		{
			ptr->~Obj();
			alloc.free(ptr);
		}
	};
	typedef AllocDeleter<typename GRHAlloc, GraphicsHandle> GRHAllocDeleter;

	GRHAlloc graphAlloc;
	std::vector<ModelReqP> createModelQueue;
	std::vector<TextureReq> createTextureQueue;
	std::vector<ModelRem> removeModelQueue;
	std::vector<TextureRem> removeTextureQueue;
	ResLoadingMap modelLoading;
	ResLoadingMap textureLoading;

	GraphicsModelResMap modelResMap;
	GraphicsTextureResMap textureResMap;

	std::recursive_mutex modelResLock;
	std::recursive_mutex textureResLock;

	IGraphics* graphics;
	GENA::ResourceCache* cache;

public:
	GraphicsCache(IGraphics* graphics, GENA::ResourceCache* cache)
		: graphics(graphics),
		cache(cache),
		graphAlloc(128)
	{
	}
	~GraphicsCache();

	void doWork();
	void clear();

	IGraphics* getGraphics() const { return graphics; }

	void asyncLoadModel(std::string modelId, ResId resId, GCreatedHandler doneCallback);
	void asyncLoadTexture(std::string textureId, ResId resId, GCreatedHandler doneCallback);

private:
	void loadModelTexture(std::string textureId, ModelReqP modelReq);
	
	void queueLoadModel(ModelReqP req)
	{
		std::lock_guard<std::recursive_mutex> lock(modelResLock);
		createModelQueue.push_back(req);
	}

	void queueLoadTexture(TextureReq req)
	{
		std::lock_guard<std::recursive_mutex> lock(textureResLock);
		createTextureQueue.push_back(req);
	}

	void queueRemoveModel(ModelRem rem)
	{
		std::lock_guard<std::recursive_mutex> lock(modelResLock);
		removeModelQueue.push_back(rem);
	}

	void queueRemoveTexture(TextureRem rem)
	{
		std::lock_guard<std::recursive_mutex> lock(textureResLock);
		removeTextureQueue.push_back(rem);
	}
};

