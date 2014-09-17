#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>

#include "PoolAllocator.h"
#include "StackAllocator.h"

#include "DataTable.h"
#include "Timer.h"

struct AllocRec
{
	bool doAlloc;
	unsigned int id;
	unsigned int time;
};

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
void runPoolAllocationTestRun(unsigned int numObjects, Allocator& allocator,
							  std::vector<void*>& allocStorage, const std::vector<AllocRec>& pattern)
{
	for (const AllocRec& rec : pattern)
	{
		if (rec.doAlloc)
		{
			allocStorage[rec.id] = allocator.alloc();
		}
		else
		{
			allocator.free(allocStorage[rec.id]);
		}
	}
}

template <class Allocator>
void runPoolAllocationTestRuns(unsigned int numObjects, Allocator& allocator, std::vector<void*>& allocStorage,
							   unsigned int numRuns, const std::vector<AllocRec>& pattern)
{
	for (unsigned int i = 0; i < numRuns; ++i)
	{
		runPoolAllocationTestRun(numObjects, allocator, allocStorage, pattern);
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
					unsigned int numRuns, Timer& timer, const std::vector<AllocRec>& pattern)
{
	timer.start();
	runPoolAllocationTestRuns(numObjects, allocator, allocStorage, numRuns, pattern);
	timer.stop();

	return (float)timer.micros() / numRuns;
}

template <class Allocator>
void timeAndRecord(unsigned int numObjects, Allocator& allocator, std::vector<void*>& allocStorage,
					unsigned int numRuns, Timer& timer, DataTable& table, unsigned int column, unsigned int row,
					const std::vector<AllocRec>& pattern)
{
	float time = timePoolTests(numObjects, allocator, allocStorage, numRuns, timer, pattern);
	table.recordValue(column, row, time);
}

template <unsigned int objectSize>
void recordRow(unsigned int numObjects, unsigned int numRuns, DataTable& table, unsigned int row, const std::vector<AllocRec>& pattern)
{
	GENA::PoolAllocator<objectSize> pool(numObjects);
	CObjectAlloc<objectSize> cPool;

	Timer t;
	std::vector<void*> storage(numObjects);
	
	table.recordValue(0, row, objectSize);
	timeAndRecord(numObjects, pool, storage, numRuns, t, table, 1, row, pattern);
	timeAndRecord(numObjects, cPool, storage, numRuns / 10, t, table, 2, row, pattern);
}

std::vector<AllocRec> generateRandomAllocPattern(unsigned int numAllocs)
{
	std::vector<AllocRec> res;
	res.reserve(numAllocs * 2);
	std::uniform_int_distribution<unsigned int> dist(0, std::numeric_limits<unsigned int>::max() / 2);
	std::default_random_engine randEng(0);

	for (unsigned int i = 0; i < numAllocs; ++i)
	{
		AllocRec aRec = {true, i, dist(randEng)};
		res.push_back(aRec);
		AllocRec dRec = {false, i, aRec.time + dist(randEng)};
		res.push_back(dRec);
	}

	std::sort(res.begin(), res.end(), 
		[] (const AllocRec& lhs, const AllocRec& rhs)
		{
			return lhs.time < rhs.time;
		});

	return res;
}

std::vector<AllocRec> generateLinearAllocPattern(unsigned int numAllocs)
{
	std::vector<AllocRec> res;
	res.reserve(numAllocs * 2);

	for (unsigned int i = 0; i < numAllocs; ++i)
	{
		AllocRec rec = {true, i, 0};
		res.push_back(rec);
	}

	for (unsigned int i = 0; i < numAllocs; ++i)
	{
		AllocRec rec = {false, i, 0};
		res.push_back(rec);
	}

	return res;
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

	std::vector<AllocRec> pattern = generateLinearAllocPattern(numObjects);

	recordRow<1>(numObjects, numRuns, table, row++, pattern);
	recordRow<2>(numObjects, numRuns, table, row++, pattern);
	recordRow<4>(numObjects, numRuns, table, row++, pattern);
	recordRow<8>(numObjects, numRuns, table, row++, pattern);
	recordRow<16>(numObjects, numRuns, table, row++, pattern);
	recordRow<32>(numObjects, numRuns, table, row++, pattern);
	recordRow<64>(numObjects, numRuns, table, row++, pattern);
	recordRow<128>(numObjects, numRuns, table, row++, pattern);
	recordRow<256>(numObjects, numRuns, table, row++, pattern);
	recordRow<512>(numObjects, numRuns, table, row++, pattern);
	recordRow<1024>(numObjects, numRuns, table, row++, pattern);
	recordRow<2048>(numObjects, numRuns, table, row++, pattern);

	table.printCSV(std::ofstream("poolAllocatorLinear.csv"));

	table = DataTable(headers);
	row = 0;

	pattern = generateRandomAllocPattern(numObjects);

	recordRow<1>(numObjects, numRuns, table, row++, pattern);
	recordRow<2>(numObjects, numRuns, table, row++, pattern);
	recordRow<4>(numObjects, numRuns, table, row++, pattern);
	recordRow<8>(numObjects, numRuns, table, row++, pattern);
	recordRow<16>(numObjects, numRuns, table, row++, pattern);
	recordRow<32>(numObjects, numRuns, table, row++, pattern);
	recordRow<64>(numObjects, numRuns, table, row++, pattern);
	recordRow<128>(numObjects, numRuns, table, row++, pattern);
	recordRow<256>(numObjects, numRuns, table, row++, pattern);
	recordRow<512>(numObjects, numRuns, table, row++, pattern);
	recordRow<1024>(numObjects, numRuns, table, row++, pattern);
	recordRow<2048>(numObjects, numRuns, table, row++, pattern);

	table.printCSV(std::ofstream("poolAllocatorRandom.csv"));
}

int main(int argc, char* argv[])
{
	testPoolAllocator();

	return 0;
};
