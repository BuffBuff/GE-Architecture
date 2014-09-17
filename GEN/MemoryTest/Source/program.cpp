#include <iostream>

#include "StackAllocator.h"

#include "PoolAllocatorTest.h"

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

int main(int argc, char* argv[])
{
	testPoolAllocator();

	return 0;
};
