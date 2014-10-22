#ifdef _DEBUG
#pragma comment(lib, "Commond.lib")
#pragma comment(lib, "Graphicsd.lib")
#else
#pragma comment(lib, "Common.lib")
#pragma comment(lib, "Graphics.lib")
#endif

#include "GraphicsCache.h"
#include "GraphicsHandle.h"
#include "ModelBinaryLoader.h"
#include "Window.h"

#include <ResourceZipFile.h>
#include <ResourceCache.h>

#include <IGraphics.h>

#include <chrono>
#include <condition_variable>
#include <forward_list>
#include <sstream>

using namespace GENA;

struct State
{
	IGraphics* graphics;
	ResourceCache* cache;
};

typedef uint32_t ResId;
void loadModelTexture(const char* p_ResourceName, ResId p_Res, void* p_UserData)
{
	State* state = (State*)p_UserData;
	std::shared_ptr<ResourceHandle> handle = state->cache->getHandle(p_Res);
	const Buffer& buff = handle->getBuffer();
	state->graphics->createTexture(p_ResourceName, buff.data(), buff.size());
}

void releaseModelTexture(const char* p_ResourceName, void* p_UserData)
{
	int dummy = 42;
}

ResId findResourceId(const char* p_ResourceName, void* p_UserData)
{
	ResourceCache* cache = (ResourceCache*)p_UserData;
	return cache->findByPath(p_ResourceName);
}

IGraphics::Buff findResourceData(ResId p_Res, void* p_UserData)
{
	ResourceCache* cache = (ResourceCache*)p_UserData;
	std::shared_ptr<ResourceHandle> handle = cache->getHandle(p_Res);
	const Buffer& buff = handle->getBuffer();
	IGraphics::Buff retBuff = { buff.data(), buff.size() };
	return retBuff;
}

struct Model
{
	std::shared_ptr<GraphicsHandle> res;
	IGraphics::InstanceId id;
};

int main(int argc, char* argv[])
{
	ResourceCache cache(30, std::unique_ptr<IResourceFile>(new ResourceZipFile("resources.bin")));
	cache.init();

	Window win;

	bool close = false;

	win.registerCallback(WM_CLOSE,
		[&close](WPARAM, LPARAM, LRESULT& res)
		{
			close = true;
			res = 0;
			return true;
		});

	win.registerCallback(WM_KEYDOWN,
		[&close](WPARAM vKey, LPARAM, LRESULT& res)
		{
			if (vKey == VK_ESCAPE)
			{
				close = true;
				res = 0;
				return true;
			}
			else
			{
				return false;
			}
		});

	win.init("Resource test program", DirectX::XMFLOAT2(800, 480));

	TweakSettings::initializeMaster();

	IGraphics* graphics = IGraphics::createGraphics();
	graphics->setResourceCallbacks(findResourceData, &cache, findResourceId, &cache);
	graphics->setTweaker(TweakSettings::getInstance());
	graphics->setShadowMapResolution(1024);
	graphics->enableShadowMap(true);
	graphics->initialize(win.getHandle(), (int)win.getSize().x, (int)win.getSize().y, false, 60.f);

	State state = { graphics, &cache };

	graphics->setLoadModelTextureCallBack(loadModelTexture, &state);
	graphics->setReleaseModelTextureCallBack(releaseModelTexture, &state);

	GraphicsCache gCache(graphics, &cache);

	ResId skyDomeId = cache.findByPath("assets/textures/Skybox1_COLOR.dds");
	std::shared_ptr<GraphicsHandle> skyDomeGRes;

	gCache.asyncLoadTexture("SKYDOME", skyDomeId,
		[&](std::shared_ptr<GraphicsHandle> res)
		{
			skyDomeGRes = res;
			graphics->createSkydome("SKYDOME", 500000.f);
		});

	std::vector<Model> objects;
	
	std::shared_ptr<ResourceHandle> room1 = cache.getHandle(2784892733);
	const Buffer& buff = room1->getBuffer();

	std::istringstream iss(std::string(buff.data(), buff.size()));
	std::string word;

	iss >> word;
	while (iss)
	{
		assert(word == "-");
		iss >> word;
		assert(word == "res:");
		ResId res = 0;
		iss >> res;
		iss >> word;
		assert(word == "x:");
		float x, y, z;
		iss >> x;
		iss >> word;
		assert(word == "y:");
		iss >> y;
		iss >> word;
		assert(word == "z:");
		iss >> z;
		iss >> word;
		
		std::string resName = cache.findPath(res);

		gCache.asyncLoadModel(resName, res,
		[=, &objects](std::shared_ptr<GraphicsHandle> res)
		{
			Model m = { res, graphics->createModelInstance(resName.c_str()) };
			objects.push_back(m);
			graphics->setModelPosition(m.id, Vector3(x, y, z));
		});
	}

	typedef std::chrono::steady_clock cl;
	cl::time_point stop = cl::now() + std::chrono::seconds(10);

	float currAngle = 0.f;

	cl::time_point currTime;
	cl::time_point prevTime = cl::now();

	uint64_t maxCacheUsage = 0;

	while (!close && (currTime = cl::now()) < stop)
	{
		cl::duration frameTime = currTime - prevTime;
		prevTime = currTime;

		float dt = std::chrono::duration<float>(frameTime).count();

		win.pollMessages();
		gCache.doWork();

		const static float angleSpeed = 1.f;
		currAngle += angleSpeed * dt;

		uint64_t newCacheUsage = cache.getMaxMemAllocated();
		if (newCacheUsage > maxCacheUsage)
		{
			maxCacheUsage = newCacheUsage;
			std::cout << "New cache max: " << maxCacheUsage << std::endl;
		}

		graphics->updateCamera(Vector3(0.f, 0.f, 0.f), Vector3(sin(currAngle), 0.f, cos(currAngle)), Vector3(0.f, 1.f, 0.f));

		graphics->useFrameDirectionalLight(Vector3(1.f, 1.f, 1.f), Vector3(-0.3f, -0.8f, 0.f), 1.f);
		graphics->useFramePointLight(Vector3(0.f, 100.f, 0.f), Vector3(1.f, 1.f, 1.f), 1000.f);

		for (const Model& m : objects)
		{
			graphics->renderModel(m.id);
		}

		if (skyDomeGRes)
		{
			graphics->renderSkydome();
		}

		graphics->drawFrame();

		std::this_thread::sleep_for(std::chrono::milliseconds(15) - frameTime);
	}

	objects.clear();

	skyDomeGRes.reset();
	gCache.clear();

	IGraphics::deleteGraphics(graphics);
	win.destroy();
}
