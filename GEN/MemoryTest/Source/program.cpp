#include <fstream>
#include <iostream>

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

template <class Allocator>
void runPoolAllocationTestRun(unsigned int numObjects, Allocator& allocator, std::vector<void*>& allocStorage)
{
	for (unsigned int i = 0; i < numObjects; ++i)
	{
		allocStorage.push_back(allocator.alloc());
	}

	for (void* alloc : allocStorage)
	{
		allocator.free(alloc);
	}
	allocStorage.clear();
}

template <class Allocator>
void runPoolAllocationTestRuns(unsigned int numObjects, Allocator& allocator, std::vector<void*>& allocStorage, unsigned int numRuns)
{
	for (unsigned int i = 0; i < numRuns; ++i)
	{
		runPoolAllocationTestRun(numObjects, allocator, allocStorage);
	}
}

template <unsigned int objectSize>
struct CObjectAlloc
{
	void* alloc()
	{
		return malloc(objectSize);
	}

	void free(void* mem)
	{
		::free(mem);
	}
};

template <class Allocator>
float timePoolTests(unsigned int numObjects, Allocator& allocator, std::vector<void*>& allocStorage,
					unsigned int numRuns, Timer& timer)
{
	timer.start();
	runPoolAllocationTestRuns(numObjects, allocator, allocStorage, numRuns);
	timer.stop();

	return (float)timer.micros() / numRuns;
}

template <class Allocator>
void timeAndRecord(unsigned int numObjects, Allocator& allocator, std::vector<void*>& allocStorage,
					unsigned int numRuns, Timer& timer, DataTable& table, unsigned int column, unsigned int row)
{
	float time = timePoolTests(numObjects, allocator, allocStorage, numRuns, timer);
	table.recordValue(column, row, time);
}

template <unsigned int objectSize>
void recordRow(unsigned int numObjects, unsigned int numRuns, DataTable& table, unsigned int row)
{
	GENA::PoolAllocator<objectSize> pool(numObjects);
	CObjectAlloc<objectSize> cPool;

	Timer t;
	std::vector<void*> storage;
	storage.reserve(numObjects);
	
	table.recordValue(0, row, objectSize);
	timeAndRecord(numObjects, pool, storage, numRuns, t, table, 1, row);
	timeAndRecord(numObjects, cPool, storage, numRuns / 10, t, table, 2, row);
}

void testPoolAllocator()
{
	std::vector<std::string> headers;
	headers.push_back("ObjectSize");
	headers.push_back("PoolAllocator");
	headers.push_back("CObjectAlloc");

	DataTable table(headers);

	const unsigned int numRuns = 500;
	const unsigned int numObjects = 1024;

	unsigned int row = 0;

	recordRow<1>(numObjects, numRuns, table, row++);
	recordRow<2>(numObjects, numRuns, table, row++);
	recordRow<4>(numObjects, numRuns, table, row++);
	recordRow<8>(numObjects, numRuns, table, row++);
	recordRow<16>(numObjects, numRuns, table, row++);
	recordRow<32>(numObjects, numRuns, table, row++);
	recordRow<64>(numObjects, numRuns, table, row++);
	recordRow<128>(numObjects, numRuns, table, row++);
	recordRow<256>(numObjects, numRuns, table, row++);
	recordRow<512>(numObjects, numRuns, table, row++);
	recordRow<1024>(numObjects, numRuns, table, row++);
	recordRow<2048>(numObjects, numRuns, table, row++);

	table.printCSV(std::ofstream("poolAllocator.csv"));
}

int main(int argc, char* argv[])
{
	testPoolAllocator();

	return 0;
};
