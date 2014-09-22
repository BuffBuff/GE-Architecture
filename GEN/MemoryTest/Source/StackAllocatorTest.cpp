#include "StackAllocatorTest.h"

#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

#include "DataTable.h"
#include "StackAllocator.h"
#include "Timer.h"

struct StackAllocRec
{
	unsigned int size;
};

struct CStackAllocator
{
	typedef uint32_t Marker;

	std::vector<void*> allocations;

	void* alloc(uint32_t sizeBytes, uint32_t unused)
	{
		allocations.push_back(malloc(sizeBytes));
		return allocations.back();
	}

	Marker getMarker()
	{
		return allocations.size();
	}

	void freeToMarker(Marker marker)
	{
		while (marker < allocations.size())
		{
			free(allocations.back());
			allocations.pop_back();
		}
	}

	void clear()
	{
		freeToMarker(0);
	}
};

template <class Allocator>
void runStackAllocationTestRun(Allocator& allocator, const std::vector<StackAllocRec>& pattern, unsigned int numObjects)
{
	Allocator::Marker marker = allocator.getMarker();
	for (unsigned int i = 0; i < numObjects; ++i)
	{
		allocator.alloc(pattern[i].size, 1);
	}
	allocator.freeToMarker(marker);
}

template <class Allocator>
float timeStackTests(Allocator& allocator, float testTimeSec, Timer& timer,
					 const std::vector<StackAllocRec>& pattern, unsigned int numObjects)
{
	typedef std::chrono::high_resolution_clock cl;

	std::chrono::milliseconds duration((unsigned int)(testTimeSec * 1000.f));
	cl::time_point stopTime = cl::now() + duration;
	unsigned int numRuns = 0;

	timer.start();
	while (cl::now() < stopTime)
	{
		runStackAllocationTestRun(allocator, pattern, numObjects);
		++numRuns;
	}
	timer.stop();

	return (float)timer.micros() / numRuns;
}

template <class Allocator>
void timeAndRecord(Allocator& allocator, float testTimeSec, Timer& timer, DataTable& table,
				   unsigned int column, unsigned int row,
				   const std::vector<StackAllocRec>& pattern, unsigned int numObjects)
{
	float time = timeStackTests(allocator, testTimeSec, timer, pattern, numObjects);
	table.recordValue(column, row, time / numObjects);
}

void runTestSet(const std::vector<StackAllocRec>& pattern, unsigned int stackSize, const std::string& filename)
{
	std::cout << "Running test set...";

	std::vector<std::string> headers;
	headers.push_back("NumObjects");
	headers.push_back("StackAllocator");
	headers.push_back("CStackAllocator");

	DataTable table(headers);

	const float runTimePerTestSec = 0.5f;
	const unsigned int maxNumObjects = pattern.size();

	GENA::StackAllocator stackAllocator(stackSize);
	CStackAllocator cStack;
	cStack.allocations.reserve(maxNumObjects);

	Timer t;

	unsigned int numObjects = 128;
	unsigned int row = 0;
	while (numObjects <= maxNumObjects)
	{
		table.recordValue(0, row, numObjects);
		timeAndRecord(stackAllocator, runTimePerTestSec, t, table, 1, row, pattern, numObjects);
		timeAndRecord(cStack, runTimePerTestSec, t, table, 2, row, pattern, numObjects);
		++row;
		numObjects *= 2;
	}

	table.printCSV(std::ofstream(filename));

	std::cout << "done.\n";
}

std::vector<StackAllocRec> generateSameSizePattern(unsigned int numObjects, unsigned int objectSize)
{
	std::cout << "Generating simple pattern\n";

	std::vector<StackAllocRec> res;
	res.reserve(numObjects);

	for (unsigned int i = 0; i < numObjects; ++i)
	{
		StackAllocRec rec = {objectSize};
		res.push_back(rec);
	}

	return res;
};

std::vector<StackAllocRec> generateRandomSizePattern(unsigned int numObjects)
{
	std::cout << "Generating random pattern\n";

	std::vector<StackAllocRec> res;
	res.reserve(numObjects);

	std::default_random_engine randEng(1);

	unsigned int sizes[] =
	{
		4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 6, 20, 33, 100
	};

	unsigned int sizeWeight[] =
	{
		200, 200, 120, 120, 60, 30, 20, 10, 7, 5, 10, 10,  10, 10
	};

	unsigned int nw = std::extent<decltype(sizeWeight)>::value;

	std::discrete_distribution<unsigned int> dist(nw, 0, 1,
		[&sizeWeight, &nw] (double val)
		{
			return (double)sizeWeight[(int)(val * nw)];
		});

	for (unsigned int i = 0; i < numObjects; ++i)
	{
		StackAllocRec rec = {sizes[dist(randEng)]};
		res.push_back(rec);
	}

	return res;
}

void testStackAllocator()
{
	const unsigned int numObjects = 1024 * 1024;
	const unsigned int objectSize = 16;
	runTestSet(generateSameSizePattern(numObjects, objectSize), numObjects * objectSize, "stackAllocatorSimple.csv");
	std::vector<StackAllocRec> randSizePattern = generateRandomSizePattern(numObjects);
	unsigned int sum = std::accumulate(randSizePattern.begin(), randSizePattern.end(), 0,
		[] (unsigned int partialSum, const StackAllocRec& val)
		{
			return partialSum + val.size;
		});
	runTestSet(randSizePattern, sum, "stackAllocatorRand.csv");
}
