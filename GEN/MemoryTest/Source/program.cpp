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
					float testTimeSec, Timer& timer, const std::vector<AllocRec>& pattern)
{
	typedef std::chrono::high_resolution_clock cl;
	
	std::chrono::milliseconds duration((unsigned int)(testTimeSec * 1000.f));
	cl::time_point stopTime = cl::now() + duration;
	unsigned int numRuns = 0;

	timer.start();
	while (cl::now() < stopTime)
	{
		runPoolAllocationTestRun(numObjects, allocator, allocStorage, pattern);
		++numRuns;
	}
	timer.stop();

	return (float)timer.micros() / numRuns;
}

template <class Allocator>
void timeAndRecord(unsigned int numObjects, Allocator& allocator, std::vector<void*>& allocStorage,
					float testTimeSec, Timer& timer, DataTable& table, unsigned int column, unsigned int row,
					const std::vector<AllocRec>& pattern)
{
	float time = timePoolTests(numObjects, allocator, allocStorage, testTimeSec, timer, pattern);
	table.recordValue(column, row, time);
}

template <unsigned int objectSize>
void recordRow(unsigned int numObjects, float testTimeSec, DataTable& table, unsigned int row, const std::vector<AllocRec>& pattern)
{
	GENA::PoolAllocator<objectSize> pool(numObjects);
	CObjectAlloc<objectSize> cPool;

	Timer t;
	std::vector<void*> storage(numObjects);
	
	table.recordValue(0, row, objectSize);
	timeAndRecord(numObjects, pool, storage, testTimeSec, t, table, 1, row, pattern);
	timeAndRecord(numObjects, cPool, storage, testTimeSec, t, table, 2, row, pattern);
}

std::vector<AllocRec> generateRandomAllocPattern(unsigned int numAllocs)
{
	std::cout << "Generating random alloc pattern\n";

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
	std::cout << "Generating linear alloc pattern\n";

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

void runTestSet(const std::vector<AllocRec>& pattern, const std::string& filename)
{
	std::cout << "Starting test set...";

	std::vector<std::string> headers;
	headers.push_back("ObjectSize");
	headers.push_back("PoolAllocator");
	headers.push_back("CObjectAlloc");

	DataTable table(headers);

	const float runTimePerTestSec = 0.5f;
	const unsigned int numObjects = 1024;

	unsigned int row = 0;

	recordRow<1>(numObjects, runTimePerTestSec, table, row++, pattern);
	recordRow<2>(numObjects, runTimePerTestSec, table, row++, pattern);
	recordRow<4>(numObjects, runTimePerTestSec, table, row++, pattern);
	recordRow<8>(numObjects, runTimePerTestSec, table, row++, pattern);
	recordRow<16>(numObjects, runTimePerTestSec, table, row++, pattern);
	recordRow<32>(numObjects, runTimePerTestSec, table, row++, pattern);
	recordRow<64>(numObjects, runTimePerTestSec, table, row++, pattern);
	recordRow<128>(numObjects, runTimePerTestSec, table, row++, pattern);
	recordRow<256>(numObjects, runTimePerTestSec, table, row++, pattern);
	recordRow<512>(numObjects, runTimePerTestSec, table, row++, pattern);
	recordRow<1024>(numObjects, runTimePerTestSec, table, row++, pattern);
	recordRow<2048>(numObjects, runTimePerTestSec, table, row++, pattern);

	table.printCSV(std::ofstream(filename));

	std::cout << "done.\n";
}

void testPoolAllocator()
{
	const unsigned int numObjects = 1024;
	runTestSet(generateLinearAllocPattern(numObjects), "poolAllocatorLinear.csv");
	runTestSet(generateRandomAllocPattern(numObjects), "poolAllocatorRandom.csv");
}

int main(int argc, char* argv[])
{
	testPoolAllocator();

	return 0;
};
