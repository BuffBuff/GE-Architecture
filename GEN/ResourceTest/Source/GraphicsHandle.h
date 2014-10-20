#pragma once

#include <memory>
#include <string>
#include <vector>

class GraphicsCache;

class GraphicsHandle
{
private:
	std::string graphicsId;
	std::string resType;
	GraphicsCache* cache;
	std::vector<std::shared_ptr<GraphicsHandle>> children;

public:
	GraphicsHandle(std::string graphicsId, std::string resType, GraphicsCache* cache)
		: graphicsId(graphicsId),
		resType(resType),
		cache(cache)
	{
	}

	~GraphicsHandle();

	void addChild(std::shared_ptr<GraphicsHandle> child);
};
