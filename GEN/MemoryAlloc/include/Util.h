#pragma once

#include <cstdint>

namespace GENA
{

	inline size_t alignOffset(size_t alignment, void* ptr)
	{
		size_t offset = (uintptr_t)ptr & (alignment - 1);
		if (offset != 0)
		{
			offset = alignment - offset;
		}

		return offset;
	}

}
