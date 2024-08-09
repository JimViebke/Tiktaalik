#pragma once

#include "immintrin.h"
#include "stdint.h"

namespace util
{
	inline uint64_t blsr(const uint64_t src) { return _blsr_u64(src); }

	inline uint64_t tzcnt(const uint64_t src) { return _tzcnt_u64(src); }

	inline uint64_t blsi(const uint64_t src) { return _blsi_u64(src); }

	inline uint64_t pdep(const uint64_t src, const uint64_t mask) { return _pdep_u64(src, mask); }

	inline uint64_t pext(const uint64_t src, const uint64_t mask) { return _pext_u64(src, mask); }

	inline constexpr uint64_t popcount(const uint64_t src) { return _mm_popcnt_u64(src); }
}
