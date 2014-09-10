#include <fstream>
#include <iostream>

#include "MemoryAlloc.h"
#include "PoolAllocator.h"
#include "StackAllocator.h"

#include "DataTable.h"
#include "Timer.h"

void testStackAllocator()
{
	GENA::StackAllocator stack(1024 * 1024);

	for (int i = 0; i < 1024; ++i)
	{
		stack.alloc(1024);
	}

	stack.clear();

	for (int i = 0; i < 1024; ++i)
	{
		stack.alloc(512);
	}

	GENA::StackAllocator::Marker marker = stack.getMarker();

	for (int i = 0; i < 1024; ++i)
	{
		stack.alloc(512);
	}

	stack.freeToMarker(marker);

	stack.alloc(512 * 1024);

	try
	{
		stack.alloc(1);
	}
	catch (std::exception& ex)
	{
		std::cout << ex.what() << std::endl;
	}
}

void testPoolAllocator()
{
	GENA::PoolAllocator<1024> pool(1024);

	struct TestObj
	{
		char data[512];
	};

	std::vector<TestObj*> objs;

	for (int i = 0; i < 1024; ++i)
	{
		objs.push_back(new(pool.alloc()) TestObj);
		objs.back()->data[13] = '@';
	}

	for (TestObj* obj : objs)
	{
		assert(obj->data[13] == '@');
		obj->~TestObj();
		pool.free(obj);
	}
}

int main(int argc, char* argv[])
{
	std::cout << GENA::getMessage() << '\n';

	Timer t;
	t.start();

	for (int i = 0; i < 1; ++i)
	{
		testStackAllocator();
		testPoolAllocator();
	}

	t.stop();

	std::cout << "Took " << t.micros() << " mics.\n";

	return 0;
};
