#include "StackAllocatorTest.h"

#include <fstream>
#include <iostream>
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

	void* alloc(uint32_t sizeBytes)
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
		while (marker > allocations.size())
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
void runStackAllocationTestRun(Allocator& allocator, const std::vector<StackAllocRec>& pattern)
{
	Allocator::Marker marker = allocator.getMarker();
	for (const StackAllocRec& rec : pattern)
	{
		allocator.alloc(rec.size);
	}
	allocator.freeToMarker(marker);
}

template <class Allocator>
float timeStackTests(Allocator& allocator, float testTimeSec, Timer& timer, const std::vector<StackAllocRec>& pattern)
{
	typedef std::chrono::high_resolution_clock cl;

	std::chrono::milliseconds duration((unsigned int)(testTimeSec * 1000.f));
	cl::time_point stopTime = cl::now() + duration;
	unsigned int numRuns = 0;

	timer.start();
	while (cl::now() < stopTime)
	{
		runStackAllocationTestRun(allocator, pattern);
		++numRuns;
	}
	timer.stop();

	return (float)timer.micros() / numRuns;
}

template <class Allocator>
void timeAndRecord(Allocator& allocator, float testTimeSec, Timer& timer, DataTable& table,
				   unsigned int column, unsigned int row,
				   const std::vector<StackAllocRec>& pattern)
{
	float time = timeStackTests(allocator, testTimeSec, timer, pattern);
	table.recordValue(column, row, time);
}

void runTestSet(const std::vector<StackAllocRec>& pattern, unsigned int stackSize, const std::string& filename)
{
	std::cout << "Running test set...";

	std::vector<std::string> headers;
	headers.push_back("ObjectSize");
	headers.push_back("StackAllocator");
	headers.push_back("CStackAllocator");

	DataTable table(headers);

	const float runTimePerTestSec = 10.f;
	const unsigned int numObjects = 1024;

	GENA::StackAllocator stackAllocator(stackSize);
	CStackAllocator cStack;
	cStack.allocations.reserve(numObjects);

	Timer t;

	table.recordValue(0, 0, "---");
	timeAndRecord(stackAllocator, runTimePerTestSec, t, table, 1, 0, pattern);
	timeAndRecord(cStack, runTimePerTestSec, t, table, 2, 0, pattern);

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

void testStackAllocator()
{
	const unsigned int numObjects = 1024;
	const unsigned int objectSize = 16;
	runTestSet(generateSameSizePattern(numObjects, objectSize), numObjects * objectSize, "stackAllocatorSimple.csv");
}
