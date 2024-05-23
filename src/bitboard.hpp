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
	extern const movemasks bishop_movemasks;
	extern const movemasks rook_movemasks;

	class bitboards
	{
	public:
		bitboard white;
		bitboard black;
	};

	inline bitboards get_bitboards_for(const position& position)
	{
		alignas(16) static constexpr std::array<uint8_t, 16> white_piece_lookup = []() consteval
		{
			std::array<uint8_t, 16> arr{};
			arr.fill(0);
			arr[white_pawn] = -1;
			arr[white_knight] = -1;
			arr[white_bishop] = -1;
			arr[white_rook] = -1;
			arr[white_queen] = -1;
			arr[white_king] = -1;
			return arr;
		}();

		bitboards bitboards;

		// load the 64 bytes of the board into two ymm registers
		const uint256_t position_lo = _mm256_loadu_si256((uint256_t*)(position._position.data() + 0));
		const uint256_t position_hi = _mm256_loadu_si256((uint256_t*)(position._position.data() + 32));

		// load the white piece mask
		const uint256_t white_piece_mask = _mm256_broadcastsi128_si256(_mm_loadu_si128((uint128_t*)&white_piece_lookup));

		// for white: replace white pieces with a ones-mask, then extract that mask
		const uint64_t white_hi = uint32_t(_mm256_movemask_epi8(_mm256_shuffle_epi8(white_piece_mask, position_hi))); // high half first
		const uint64_t white_lo = uint32_t(_mm256_movemask_epi8(_mm256_shuffle_epi8(white_piece_mask, position_lo)));
		// for black: shift color bits to the highest bit in their byte, then extract those bits into a mask
		const uint64_t black_hi = uint32_t(_mm256_movemask_epi8(_mm256_slli_epi64(position_hi, 7))); // high half first
		const uint64_t black_lo = uint32_t(_mm256_movemask_epi8(_mm256_slli_epi64(position_lo, 7)));

		// merge 2x 32-bit bitmasks to 1x 64-bit bitmask
		bitboards.white = (white_hi << 32) | white_lo;
		bitboards.black = (black_hi << 32) | black_lo;

		return bitboards;
	}

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

	template<piece_t piece_type, typename generate_moves_fn_t, typename king_check_fn_t>
	__forceinline void find_moves_for(size_t& out_index, const size_t parent_idx,
									  const size_t king_index, const tt::key key, generate_moves_fn_t generate_moves_fn, king_check_fn_t king_check_fn)
	{
		const position& parent_position = positions[parent_idx];
		bitboard pieces = get_bitboard_for<piece_type>(parent_position);

		while (pieces)
		{
			const size_t piece_idx = get_next_bit(pieces);
			pieces = clear_next_bit(pieces);

			// XOR the key for the leaving piece once for all of its moves
			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[piece_idx][piece_type];

			generate_moves_fn(out_index, parent_idx, piece_idx / 8, piece_idx % 8, king_index, incremental_key, king_check_fn);
		}
	}
}
