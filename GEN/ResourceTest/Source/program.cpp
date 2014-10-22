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
#include "RoomResourceLoader.h"
#include "Window.h"

#include <ResourceZipFile.h>
#include <ResourceCache.h>

#include <IGraphics.h>

#include <chrono>
#include <condition_variable>
#include <forward_list>
#include <sstream>

#include <vld.h>

#define SET_DBG_FLAG
#define DUMP_LEAKS

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
	SET_DBG_FLAG;
	
	ResourceCache cache(60, std::unique_ptr<IResourceFile>(new ResourceZipFile("resources.bin")));
	cache.init();
	cache.registerLoader(std::shared_ptr<IResourceLoader>(new RoomResourceLoader()));

	Window win;

	bool close = false;

	bool goForward = false;
	bool goBackward = false;

	int mouseX = 0;
	int mouseY = 0;

	win.registerCallback(WM_CLOSE,
		[&close](WPARAM, LPARAM, LRESULT& res)
		{
			close = true;
			res = 0;
			return true;
		});

	win.registerCallback(WM_KEYDOWN,
		[&](WPARAM vKey, LPARAM, LRESULT& res)
		{
			switch (vKey)
			{
			case VK_ESCAPE:
				close = true;
				res = 0;
				return true;
			
			case 'W':
				goForward = true;
				res = 0;
				return true;

			case 'S':
				goBackward = true;
				res = 0;
				return true;

			default:
				return false;
			}
		});

	win.registerCallback(WM_KEYUP,
		[&](WPARAM vKey, LPARAM, LRESULT& res)
		{
			switch (vKey)
			{
			case 'W':
				goForward = false;
				res = 0;
				return true;

			case 'S':
				goBackward = false;
				res = 0;
				return true;

			default:
				return false;
			}
		});

	win.registerCallback(WM_MOUSEMOVE,
		[&](WPARAM wParam, LPARAM lParam, LRESULT& res)
		{
			mouseX = (short)(lParam & 0x0000ffff);
			mouseY = (short)((lParam & 0xffff0000) >> 16);
			res = 0;
			return true;
		});

	bool active = true;
	POINT winCenterPos;
	win.registerCallback(WM_ACTIVATE,
		[&](WPARAM wParam, LPARAM, LRESULT& res)
		{
			bool prevActive = active;
			active = (wParam != 0);
			if (active != prevActive)
			{
				ShowCursor(!active);
				POINT screenMousePos = winCenterPos;
				ClientToScreen(win.getHandle(), &screenMousePos);
				SetCursorPos(screenMousePos.x, screenMousePos.y);
			}
			res = 0;
			return true;
		});

	win.init("Resource test program", DirectX::XMFLOAT2(800, 480));
	ShowCursor(FALSE);

	TweakSettings::initializeMaster();

	IGraphics* graphics = IGraphics::createGraphics();
	graphics->setResourceCallbacks(findResourceData, &cache, findResourceId, &cache);
	graphics->setTweaker(TweakSettings::getInstance());
	graphics->setShadowMapResolution(1024);
	graphics->enableShadowMap(true);
	graphics->initialize(win.getHandle(), (int)win.getSize().x, (int)win.getSize().y, false, 60.f);

	RECT rect;
	GetWindowRect(win.getHandle(), &rect);
	SetCursorPos((rect.right + rect.left) / 2, (rect.top + rect.bottom) / 2);
	GetCursorPos(&winCenterPos);
	ScreenToClient(win.getHandle(), &winCenterPos);
	mouseX = winCenterPos.x;
	mouseY = winCenterPos.y;

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

	uint32_t numObjs = *(uint32_t*)buff.data();
	const RoomObject* readObjPos = (RoomObject*)(buff.data() + sizeof(uint32_t));

	for (uint32_t i = 0; i < numObjs; ++i)
	{
		std::string resName = cache.findPath(readObjPos->id);
		float x = readObjPos->x;
		float y = readObjPos->y;
		float z = readObjPos->z;

		gCache.asyncLoadModel(resName, readObjPos->id,
		[=, &objects](std::shared_ptr<GraphicsHandle> res)
		{
			Model m = { res, graphics->createModelInstance(resName.c_str()) };
			objects.push_back(m);
			graphics->setModelPosition(m.id, Vector3(x, y, z));
			std::cout << "Created an instance of " << resName << " at (" << x << ", " << y << ", " << z << ")\n";
		});

		++readObjPos;
	}

	typedef std::chrono::steady_clock cl;

	float xPos = 0;

	float yaw = 0;
	float pitch = 0;

	cl::time_point currTime;
	cl::time_point prevTime = cl::now();

	uint64_t maxCacheUsage = 0;

	while (!close)
	{
		currTime = cl::now();
		cl::duration frameTime = currTime - prevTime;
		prevTime = currTime;

		float dt = std::chrono::duration<float>(frameTime).count();

		win.pollMessages();
		gCache.doWork();

		if (active)
		{
			int dX = mouseX - winCenterPos.x;
			int dY = mouseY - winCenterPos.y;
			POINT screenMousePos = winCenterPos;
			ClientToScreen(win.getHandle(), &screenMousePos);
			SetCursorPos(screenMousePos.x, screenMousePos.y);

			const static float mouseSense = 0.01f;

			yaw += dX * mouseSense;
			pitch += dY * mouseSense;
			if (pitch > PI * 0.25f)
			{
				pitch = PI * 0.25f;
			}
			else if (pitch < -PI * 0.25f)
			{
				pitch = -PI * 0.25f;
			}
		}

		float direction = 0.f;
		if (goForward) direction += 1.f;
		if (goBackward) direction -= 1.f;

		const static float moveSpeed = 500.f;
		xPos += dt * direction * moveSpeed;

		uint64_t newCacheUsage = cache.getMaxMemAllocated();
		if (newCacheUsage > maxCacheUsage)
		{
			maxCacheUsage = newCacheUsage;
			std::cout << "New cache max: " << maxCacheUsage << std::endl;
		}

		float cosP = cos(pitch);
		graphics->updateCamera(Vector3(xPos, 0.f, 0.f), Vector3(sin(yaw) * cosP, -sin(pitch), cos(yaw) * cosP), Vector3(0.f, 1.f, 0.f));

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

	for (Model& m : objects)
	{
		graphics->eraseModelInstance(m.id);
		std::cout << "Removed model instance\n";
	}
	objects.clear();

	skyDomeGRes.reset();
	gCache.clear();

	IGraphics::deleteGraphics(graphics);
	win.destroy();

	DUMP_LEAKS;
}
