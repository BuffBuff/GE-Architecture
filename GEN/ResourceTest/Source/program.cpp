#ifdef _DEBUG
#pragma comment(lib, "Commond.lib")
#pragma comment(lib, "Graphicsd.lib")
#else
#pragma comment(lib, "Common.lib")
#pragma comment(lib, "Graphics.lib")
#endif

#include "Window.h"

#include <ResourceBinFile.h>
#include <ResourceCache.h>

#include <IGraphics.h>

#include <chrono>
#include <condition_variable>

using namespace GENA;

typedef uint32_t ResId;
void loadModelTexture(const char* p_ResourceName, ResId p_Res, void* p_UserData)
{
	IGraphics* graphics = (IGraphics*)p_UserData;
	graphics->createTexture(p_ResourceName, p_Res, "dds");
}

void releaseModelTexture(ResId p_Res, void* p_UserData)
{
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

std::mutex loaded;
std::condition_variable onLoaded;
std::shared_ptr<ResourceHandle> loadedHandle;

void completionHandle(std::shared_ptr<ResourceHandle> handle, void* userData)
{
	std::unique_lock<std::mutex> lock(loaded);
	loadedHandle = handle;
	onLoaded.notify_one();
}

int main(int argc, char* argv[])
{
	ResourceCache cache(30, std::unique_ptr<IResourceFile>(new ResourceBinFile("resources.bin")));
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

	std::unique_lock<std::mutex> lock(loaded);
	cache.preload(cache.findByPath("assets/models/Barrel1.btx"), completionHandle, nullptr);
	onLoaded.wait(lock, []{ return loadedHandle; });

	IGraphics* graphics = IGraphics::createGraphics();
	graphics->setResourceCallbacks(findResourceData, &cache, findResourceId, &cache);
	graphics->setTweaker(TweakSettings::getInstance());
	graphics->setShadowMapResolution(1024);
	graphics->enableShadowMap(true);
	graphics->initialize(win.getHandle(), (int)win.getSize().x, (int)win.getSize().y, false, 60.f);

	graphics->setLoadModelTextureCallBack(loadModelTexture, graphics);
	graphics->setReleaseModelTextureCallBack(releaseModelTexture, graphics);

	ResId skyDomeId = cache.findByPath("assets/textures/Skybox1_COLOR.dds");
	graphics->createTexture("SKYDOME", skyDomeId, "dds");
	graphics->createSkydome("SKYDOME", 500000.f);

	ResId barrelId = cache.findByPath("assets/models/Barrel1.btx");
	graphics->createModel("Barrel", barrelId);

	IGraphics::InstanceId barrel = graphics->createModelInstance("Barrel");
	graphics->setModelPosition(barrel, Vector3(0.f, 0.f, 200.f));

	typedef std::chrono::steady_clock cl;
	cl::time_point stop = cl::now() + std::chrono::seconds(10);

	float currAngle = 0.f;

	cl::time_point currTime;
	cl::time_point prevTime = cl::now();

	while (!close && (currTime = cl::now()) < stop)
	{
		cl::duration frameTime = currTime - prevTime;
		prevTime = currTime;

		float dt = std::chrono::duration<float>(frameTime).count();

		win.pollMessages();

		const static float angleSpeed = 1.f;
		currAngle += angleSpeed * dt;

		graphics->updateCamera(Vector3(0.f, 0.f, 0.f), Vector3(sin(currAngle), 0.f, cos(currAngle)), Vector3(0.f, 1.f, 0.f));

		graphics->useFrameDirectionalLight(Vector3(1.f, 1.f, 1.f), Vector3(-0.3f, -0.8f, 0.f), 1.f);
		graphics->useFramePointLight(Vector3(0.f, 100.f, 0.f), Vector3(1.f, 1.f, 1.f), 1000.f);

		graphics->renderModel(barrel);

		graphics->renderSkydome();

		graphics->drawFrame();
	}

	IGraphics::deleteGraphics(graphics);
	win.destroy();
}
