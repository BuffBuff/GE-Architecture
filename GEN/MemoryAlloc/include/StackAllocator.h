#include <cstdint>

namespace GENA
{
	class StackAllocator
	{
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
		void* alloc(uint32_t sizeBytes);

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
	};
}
