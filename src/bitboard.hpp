#pragma once

#include "piece.hpp"
#include "position.hpp"
#include "types.hpp"
#include "util/strong_alias.hpp"

namespace chess
{
	using bitboard = ::util::strong_alias<uint64_t, struct bitboard_tag>;

	template<piece_t piece>
	__forceinline bitboard get_bitboard_for(const position& position)
	{
		static_assert(sizeof(chess::position) == 64);
		static_assert(alignof(chess::position) == 64);

		// load the 64 bytes of the board into two ymm registers
		uint256_t ymm0 = _mm256_loadu_si256((uint256_t*)(position._position.data() + 0));
		uint256_t ymm1 = _mm256_loadu_si256((uint256_t*)(position._position.data() + 32));

		// broadcast the target piece (type | color) to all positions of a register
		const uint256_t piece_mask = _mm256_set1_epi8(piece);

		// find the matching bytes
		ymm1 = _mm256_cmpeq_epi8(ymm1, piece_mask); // high half first
		ymm0 = _mm256_cmpeq_epi8(ymm0, piece_mask);
		// extract 2x 32-bit bitmasks
		const uint64_t mask_high = uint32_t(_mm256_movemask_epi8(ymm1)); // high half first
		const uint64_t mask_low = uint32_t(_mm256_movemask_epi8(ymm0));
		// merge 2x 32-bit bitmasks to 1x 64-bit bitmask
		const uint64_t mask = (mask_high << 32) | mask_low;

		return mask;
	}
}
