#pragma once

#include "immintrin.h"
#include "stdint.h"

namespace util
{
	inline uint64_t blsr(const uint64_t src)
	{
		return _blsr_u64(src);
	}

	inline uint64_t tzcnt(const uint64_t src)
	{
		return _tzcnt_u64(src);
	}
}
