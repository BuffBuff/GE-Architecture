#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

#include "SpinLock.h"

namespace GENA
{
	class StackAllocator
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
		explicit StackAllocator(uint32_t stackSizeBytes);

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

	private:
		std::vector<char> buffer;
		SpinLock spin;
	};

	inline StackAllocator::StackAllocator(uint32_t stackSizeBytes)
	{
		buffer.reserve(stackSizeBytes);
	}

	inline size_t alignOffset(size_t alignment, void* ptr)
	{
		size_t offset = (uintptr_t)ptr & (alignment - 1);
		if (offset != 0)
		{
			offset = alignment - offset;
		}

		return offset;
	}

	inline void* StackAllocator::alloc(uint32_t sizeBytes, uint32_t alignment)
	{
		std::lock_guard<SpinLock> lock(spin);

		void* currPos = buffer.data() + buffer.size();
		size_t offset = alignOffset(alignment, currPos);

		if (buffer.capacity() - buffer.size() - offset < sizeBytes)
		{
			throw std::exception("No more stack memory for you!");
		}

		buffer.resize(buffer.size() + sizeBytes + offset);

		return (char*)currPos + offset;
	}

	inline StackAllocator::Marker StackAllocator::getMarker()
	{
		std::lock_guard<SpinLock> lock(spin);

		return buffer.size();
	}

	inline void StackAllocator::freeToMarker(Marker marker)
	{
		std::lock_guard<SpinLock> lock(spin);

		buffer.resize(marker);
	}

	inline void StackAllocator::clear()
	{
		std::lock_guard<SpinLock> lock(spin);

		buffer.clear();
	}
}
