#pragma once

#include "GraphicsHandle.h"

#include <ResourceCache.h>
#include <ResourceHandle.h>

#include <atomic>
#include <functional>
#include <forward_list>
#include <map>
#include <memory>
#include <string>
#include <vector>

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
	std::string filetype;
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

class GraphicsCache
{
public:
	typedef GENA::ResourceHandle::ResId ResId;

private:
	friend class GraphicsHandle;

	typedef std::map<std::string, std::weak_ptr<GraphicsHandle>> GraphicsModelResMap;
	typedef std::map<std::string, std::weak_ptr<GraphicsHandle>> GraphicsTextureResMap;

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
		cache(cache)
	{
	}
	~GraphicsCache();

	void doWork();
	void clear();

	IGraphics* getGraphics() const { return graphics; }

	void asyncLoadModel(std::string modelId, ResId resId, GCreatedHandler doneCallback);
	void asyncLoadTexture(std::string textureId, ResId resId, GCreatedHandler doneCallback);

	void waitForModel(std::string modelId)
	{
		// check if in queue?
		while (modelResMap.count(modelId) == 0)
		{
			doWork();
		}
	}

	void waitForTexture(std::string textureId)
	{
		// check if in queue?
		while (textureResMap.count(textureId) == 0)
		{
			doWork();
		}
	}

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
