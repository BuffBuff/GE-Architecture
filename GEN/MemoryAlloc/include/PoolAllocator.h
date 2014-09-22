#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

#include "SpinLock.h"

namespace GENA
{
	template <unsigned int chunkSize>
	class PoolAllocator
	{
	private:

		template <int size, int aligned_size = 1>
		struct rec_find_size
		{
			static const int value = (size <= aligned_size) ? aligned_size : rec_find_size<size, aligned_size * 2>::value;
		};

		template <int size>
		struct rec_find_size<size, sizeof(std::max_align_t)>
		{
			static const int value = sizeof(std::max_align_t);
		};

		template <int size>
		struct find_align_type
		{
			typedef std::max_align_t align_type;
		};
		template <> struct find_align_type<1> {	typedef std::uint8_t align_type; };
		template <> struct find_align_type<2> {	typedef std::uint16_t align_type; };
		template <> struct find_align_type<4> {	typedef std::uint32_t align_type; };
		template <> struct find_align_type<8> { typedef std::uint64_t align_type; };

		typedef typename find_align_type<rec_find_size<chunkSize>::value>::align_type align_type_t;

	public:
		/**
		 * Constructs a pool allocator with a fixed pool size.
		 */
		PoolAllocator(uint32_t nrOfChunks)
		{
			freeList = nullptr;

			memoryBuffer.resize(nrOfChunks);
			for (unsigned int i = 0; i < nrOfChunks; ++i)
			{
				Chunk& chunk = memoryBuffer[i];
				chunk.next = freeList;
				freeList = &chunk;
			}
		};

		/**
		 * Allocates a new block from the pool.
		 */
		void* alloc()
		{
			std::lock_guard<SpinLock> lock(spin);

			if (!freeList)
			{
				throw new std::exception("No more pool memory for you!");
			}

			void* mem = (void*)freeList;
			freeList = freeList->next;

			return mem;
		}

		/**
		 * Free the memory of a chunk previously allocated from this pool.
		 */
		void free(void* mem)
		{
			std::lock_guard<SpinLock> lock(spin);

			if (!mem)
			{
				return;
			}

			Chunk* chunk = (Chunk*)mem;
			chunk->next = freeList;
			freeList = chunk;
		}

	private:
		struct Chunk
		{
			union
			{
				Chunk* next;
				char data[chunkSize];
				align_type_t forceAlignment;
			};
		};

		std::vector<Chunk> memoryBuffer;
		Chunk* freeList;
		
		SpinLock spin;
	};
}