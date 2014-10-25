#pragma once

#include <cstdint>
#include <vector>

#include "Util.h"

namespace GENA
{
	class StackAllocatorSingleThreaded
	{
	public:
		/**
		 * Stack marker: Represents the current top of the
		 * stack. You can only roll back to a marker, not to
		 * arbitrary locations within the stack.
		 */
		typedef uint32_t Marker;

		/**
		 * Constructs a stack allocator with the given total
		 * size.
		 */
		explicit StackAllocatorSingleThreaded(uint32_t stackSizeBytes);

		/**
		 * Allocates a new block of the given size from stack
		 * top.
		 */
		void* alloc(uint32_t sizeBytes, uint32_t alignment = sizeof(std::max_align_t));

		/**
		 * Returns a marker to the current top.
		 */
		Marker getMarker();

		/**
		 * Rolls the stack back to a previous marker.
		 */
		void freeToMarker(Marker marker);

		/**
		 * Clears the entire stack (rolls the stack back to
		 * zero).
		 */
		void clear();

		/**
		 * Get the maximum stack size used since the stack was created.
		 */
		size_t getMaxAllocated() const;

	private:
		std::vector<char> buffer;
		size_t maxAllocated;
	};

	inline StackAllocatorSingleThreaded::StackAllocatorSingleThreaded(uint32_t stackSizeBytes)
		: maxAllocated(0)
	{
		buffer.reserve(stackSizeBytes);
	}

	inline void* StackAllocatorSingleThreaded::alloc(uint32_t sizeBytes, uint32_t alignment)
	{
		void* currPos = buffer.data() + buffer.size();
		size_t offset = alignOffset(alignment, currPos);

		if (buffer.capacity() - buffer.size() - offset < sizeBytes)
		{
			throw std::exception("No more stack memory for you!");
		}

		buffer.resize(buffer.size() + sizeBytes + offset);

		if (buffer.size() > maxAllocated)
		{
			maxAllocated = buffer.size();
		}

		return (char*)currPos + offset;
	}

	inline StackAllocatorSingleThreaded::Marker StackAllocatorSingleThreaded::getMarker()
	{
		return buffer.size();
	}

	inline void StackAllocatorSingleThreaded::freeToMarker(Marker marker)
	{
		buffer.resize(marker);
	}

	inline void StackAllocatorSingleThreaded::clear()
	{
		buffer.clear();
	}

	inline size_t StackAllocatorSingleThreaded::getMaxAllocated() const
	{
		return maxAllocated;
	}
}
