#include "GraphicsCache.h"

#include "ModelBinaryLoader.h"

#include <IGraphics.h>

using namespace GENA;

GraphicsCache::~GraphicsCache()
{
	clear();
}

void GraphicsCache::doWork()
{
	{
		std::lock_guard<std::recursive_mutex> lock(textureResLock);

		for (auto& texReq : createTextureQueue)
		{
			const Buffer& buff = texReq.resource->getBuffer();
			graphics->createTexture(texReq.textureId.c_str(), buff.data(), buff.size(), texReq.filetype.c_str());
			
			if (textureResMap.count(texReq.textureId) > 0)
			{
				throw std::runtime_error("Texture " + texReq.textureId + " already loaded");
			}

			std::shared_ptr<GraphicsHandle> resHandle(new GraphicsHandle(texReq.textureId, "Texture", this));
			
			textureResMap[texReq.textureId] = resHandle;
			
			GCreatedHandlers complete = std::move(textureLoading[texReq.textureId]);
			textureLoading.erase(texReq.textureId);
			for (auto handler : complete)
			{
				handler(resHandle);
			}
		}
		createTextureQueue.clear();
	}

	{
		std::lock_guard<std::recursive_mutex> lock(modelResLock);

		std::vector<ModelReqP> savedModReq;
		for (auto& modReq : createModelQueue)
		{
			if (modReq->texturesToLoad > 0)
			{
				savedModReq.push_back(std::move(modReq));
				continue;
			}

			const Buffer& buff = modReq->resource->getBuffer();

			ModelBinaryLoader loader;
			loader.loadBinaryFromMemory(buff.data(), buff.size());

			std::vector<CMaterial> mats;
			for (const Material& mat : loader.getMaterial())
			{
				mats.push_back(CMaterial());
				mats.back().m_DiffuseMap = mat.m_DiffuseMap.c_str();
				mats.back().m_NormalMap = mat.m_NormalMap.c_str();
				mats.back().m_SpecularMap = mat.m_SpecularMap.c_str();
			}
			std::vector<CMaterialBuffer> matBuffs;
			for (const MaterialBuffer& matBuf : loader.getMaterialBuffer())
			{
				matBuffs.push_back(CMaterialBuffer());
				matBuffs.back().start = matBuf.start;
				matBuffs.back().length = matBuf.length;
			}

			const void* vertData;
			size_t vertSize;
			size_t numVerts;

			if (loader.getAnimated())
			{
				vertData = loader.getAnimatedVertexBuffer().data();
				vertSize = sizeof(AnimatedVertex);
				numVerts = loader.getAnimatedVertexBuffer().size();
			}
			else
			{
				vertData = loader.getStaticVertexBuffer().data();
				vertSize = sizeof(StaticVertex);
				numVerts = loader.getStaticVertexBuffer().size();
			}

			graphics->createModel(modReq->modelId.c_str(), mats.data(), mats.size(), matBuffs.data(), matBuffs.size(),
				loader.getAnimated(), loader.getTransparent(), vertData, vertSize, numVerts, loader.getBoundingVolume().data());
			
			if (modelResMap.count(modReq->modelId) > 0)
			{
				throw std::runtime_error("Model " + modReq->modelId + " already loaded");
			}

			std::shared_ptr<GraphicsHandle> resHandle(new GraphicsHandle(modReq->modelId, "Model", this));
			for (auto child : modReq->children)
			{
				resHandle->addChild(child);
			}

			modelResMap[modReq->modelId] = resHandle;
			
			GCreatedHandlers complete = std::move(modelLoading[modReq->modelId]);
			modelLoading.erase(modReq->modelId);
			for (auto handler : complete)
			{
				handler(resHandle);
			}
		}
		std::swap(createModelQueue, savedModReq);
	}

	{
		std::lock_guard<std::recursive_mutex> lock(modelResLock);

		for (auto& modRem : removeModelQueue)
		{
			graphics->releaseModel(modRem.modelId.c_str());
						
			if (modRem.completionHandler)
			{
				modRem.completionHandler(modRem.modelId);
			}
		}
		removeModelQueue.clear();
	}

	{
		std::lock_guard<std::recursive_mutex> lock(textureResLock);

		for (auto& texRem : removeTextureQueue)
		{
			graphics->releaseTexture(texRem.textureId.c_str());
						
			if (texRem.completionHandler)
			{
				texRem.completionHandler(texRem.textureId);
			}
		}
		removeTextureQueue.clear();
	}
}

void GraphicsCache::clear()
{
	modelResMap.clear();
	textureResMap.clear();
	doWork();
}

typedef std::function<void(std::shared_ptr<ResourceHandle>)> CompletionHandler;

static void completionHelper(std::shared_ptr<ResourceHandle> resource, void* userData)
{
	CompletionHandler* handler = (CompletionHandler*)userData;
	(*handler)(resource);
	delete handler;
}

void GraphicsCache::asyncLoadModel(std::string modelId, ResId resId, GCreatedHandler doneCallback)
{
	std::lock_guard<std::recursive_mutex> lock(modelResLock);

	if (modelResMap.count(modelId) > 0)
	{
		std::shared_ptr<GraphicsHandle> resHandle = modelResMap[modelId].lock();
		if (resHandle)
		{
			if (doneCallback)
			{
				doneCallback(resHandle);
			}
			return;
		}
		else
		{
			modelResMap.erase(modelId);
		}
	}

	{
		bool startLoading = (modelLoading.count(modelId) == 0);

		modelLoading[modelId].push_front(doneCallback);

		if (startLoading)
		{
			CompletionHandler* ch = new CompletionHandler(
				[=](std::shared_ptr<ResourceHandle> resource)
				{
					const Buffer& buff = resource->getBuffer();

					ModelBinaryLoader loader;
					loader.loadBinaryFromMemory(buff.data(), buff.size());
					
					ModelReqP req(new ModelReq(modelId, resource));

					for (const auto& mat : loader.getMaterial())
					{
						loadModelTexture(mat.m_DiffuseMap, req);
						loadModelTexture(mat.m_NormalMap, req);
						loadModelTexture(mat.m_SpecularMap, req);
					}

					queueLoadModel(req);
				});
			cache->preload(resId, completionHelper, ch);
		}
	}
}

void GraphicsCache::asyncLoadTexture(std::string textureId, ResId resId, GCreatedHandler doneCallback)
{
	std::lock_guard<std::recursive_mutex> lock(textureResLock);

	if (textureResMap.count(textureId) > 0)
	{
		std::shared_ptr<GraphicsHandle> resHandle = textureResMap[textureId].lock();

		if (resHandle)
		{
			if (doneCallback)
			{
				doneCallback(resHandle);
			}
			return;
		}
		else
		{
			textureResMap.erase(textureId);
		}
	}

	{
		bool startLoading = (textureLoading.count(textureId) == 0);

		textureLoading[textureId].push_front(doneCallback);

		if (startLoading)
		{
			CompletionHandler* ch = new CompletionHandler(
				[=](std::shared_ptr<ResourceHandle> resource)
				{
					TextureReq req = { textureId, resource, "dds" };
					queueLoadTexture(req);
				});
			cache->preload(resId, completionHelper, ch);
		}
	}
}

void GraphicsCache::loadModelTexture(std::string textureId, ModelReqP modelReq)
{
	std::lock_guard<std::recursive_mutex> lock(textureResLock);

	std::shared_ptr<GraphicsHandle> resHandle;
	if (textureResMap.count(textureId) == 0 || !(resHandle = textureResMap[textureId].lock()))
	{
		++(modelReq->texturesToLoad);

		ResId resId = cache->findByPath("assets/textures/" + textureId);
		GCreatedHandler doneCallback =
			[=](std::shared_ptr<GraphicsHandle> resHandle)
			{
				modelReq->children.push_back(resHandle);
				--(modelReq->texturesToLoad);
			};

		bool startLoading = (textureLoading.count(textureId) == 0);

		textureLoading[textureId].push_front(doneCallback);

		if (startLoading)
		{
			CompletionHandler* ch = new CompletionHandler(
				[=](std::shared_ptr<ResourceHandle> resource)
			{
				TextureReq req = { textureId, resource, "dds" };
				queueLoadTexture(req);
			});
			cache->preload(resId, completionHelper, ch);
		}
	}
	else
	{
		modelReq->children.push_back(resHandle);
	}
}