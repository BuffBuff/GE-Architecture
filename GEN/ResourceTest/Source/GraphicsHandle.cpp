#include "GraphicsHandle.h"

#include "GraphicsCache.h"

GraphicsHandle::~GraphicsHandle()
{
	if (resType == "Model")
	{
		ModelRem rem = { graphicsId };
		cache->queueRemoveModel(rem);
	}
	else if (resType == "Texture")
	{
		TextureRem rem = { graphicsId };
		cache->queueRemoveTexture(rem);
	}
}

void GraphicsHandle::addChild(std::shared_ptr<GraphicsHandle> child)
{
	children.push_back(child);
}
