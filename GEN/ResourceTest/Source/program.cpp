#ifdef _DEBUG
#pragma comment(lib, "Commond.lib")
#pragma comment(lib, "Graphicsd.lib")
#else
#pragma comment(lib, "Common.lib")
#pragma comment(lib, "Graphics.lib")
#endif

#include "ModelBinaryLoader.h"
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

struct ResourceLoad
{
	ResId res;
	std::string type;
	std::string name;
};
std::vector<std::pair<std::shared_ptr<ResourceHandle>, ResourceLoad>> loadedHandles;

std::vector<ResourceLoad> resLoads;

void completionHandle(std::shared_ptr<ResourceHandle> handle, void* userData)
{
	std::unique_lock<std::mutex> lock(loaded);
	loadedHandles.push_back(std::make_pair(handle, *((ResourceLoad*)userData)));

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
	resLoads.push_back(ResourceLoad());
	resLoads.back().res = barrelId;
	resLoads.back().type = "Model";
	resLoads.back().name = "Barrel";

	std::unique_lock<std::mutex> lock(loaded);

	for (auto& res : resLoads)
	{
		cache.preload(res.res, completionHandle, &res);
	}

	size_t resourcesToLoad = resLoads.size();
	while (resourcesToLoad > 0)
	{
		onLoaded.wait(lock, []{ return !loadedHandles.empty(); });

		auto handle = loadedHandles.back();
		loadedHandles.pop_back();
		
		--resourcesToLoad;

		const Buffer& buff = handle.first->getBuffer();

		if (handle.second.type == "Model")
		{
			ModelBinaryLoader loader;
			loader.loadBinaryFromMemory(buff.data(), buff.size());

			for (const auto& mat : loader.getMaterial())
			{
				ResId diffId = cache.findByPath("assets/textures/" + mat.m_DiffuseMap);
				graphics->createTexture(mat.m_DiffuseMap.c_str(), diffId, "dds");
				ResId normId = cache.findByPath("assets/textures/" + mat.m_NormalMap);
				graphics->createTexture(mat.m_NormalMap.c_str(), normId, "dds");
				ResId specId = cache.findByPath("assets/textures/" + mat.m_SpecularMap);
				graphics->createTexture(mat.m_SpecularMap.c_str(), specId, "dds");
			}

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

			graphics->createModel(handle.second.name.c_str(), mats.data(), mats.size(), matBuffs.data(), matBuffs.size(),
				loader.getAnimated(), loader.getTransparent(), vertData, vertSize, numVerts, loader.getBoundingVolume().data());
		}
	}

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
