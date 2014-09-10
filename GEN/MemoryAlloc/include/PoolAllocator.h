#pragma once

#include <cstdint>
#include <vector>

namespace GENA
{
	template <unsigned int chunkSize>
	class PoolAllocator
	{
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
			};
		};

		std::vector<Chunk> memoryBuffer;
		Chunk* freeList;
	};
}