#pragma once

#include <array>

#include "piece.hpp"
#include "position.hpp"
#include "types.hpp"
#include "util/intrinsics.hpp"
#include "util/strong_alias.hpp"

namespace chess
{
	using bitboard = ::util::strong_alias<uint64_t, struct bitboard_tag>;

	void print_bitboard(const bitboard bitboard);

	using movemasks = std::array<bitboard, 64>;

	extern const movemasks knight_movemasks;

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

	inline size_t get_next_bit(const bitboard bitboard)
	{
		return ::util::tzcnt(bitboard);
	}

	inline bitboard clear_next_bit(const bitboard bitboard)
	{
		return ::util::blsr(bitboard);
	}

	template<piece_t piece_type, typename parent_node_t, typename generate_moves_fn_t, typename king_check_fn_t>
	__forceinline void find_moves_for(size_t& out_index, parent_node_t& parent_node, const position& position,
									  const size_t king_index, const tt::key key, generate_moves_fn_t generate_moves_fn, king_check_fn_t king_check_fn)
	{
		bitboard pieces = get_bitboard_for<piece_type>(position);

		while (pieces)
		{
			const size_t piece_idx = get_next_bit(pieces);
			pieces = clear_next_bit(pieces);

			// XOR the key for the leaving piece once for all of its moves
			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[piece_idx][piece_type];

			generate_moves_fn(out_index, parent_node, position, piece_idx / 8, piece_idx % 8, king_index, incremental_key, king_check_fn);
		}
	}
}
