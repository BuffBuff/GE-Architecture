#include <ResourceBinFile.h>
#include <ResourceCache.h>

#include <fstream>

int main(int argc, char* argv[])
{
	typedef GENA::ResourceCache Res;

	std::unique_ptr<GENA::ResourceBinFile> file(new GENA::ResourceBinFile("resources.bin"));

	Res cache(50, std::move(file));
	cache.init();

	Res::ResId testRes = 369115351;
	std::shared_ptr<GENA::ResourceHandle> handle = cache.getHandle(testRes);
	const GENA::Buffer& buffer = handle->getBuffer();

	std::ofstream("out.png").write(buffer.data(), buffer.size());

	return 0;
}
