#pragma once

#include <array>
#include <memory>

#include "config.hpp"
#include "piece.hpp"
#include "position.hpp"
#include "types.hpp"
#include "util/intrinsics.hpp"
#include "util/strong_alias.hpp"

namespace chess
{
	using bitboard = ::util::strong_alias<uint64_t, struct bitboard_tag>;

	void print_bitboard(const bitboard bitboard);

	extern const std::array<bitboard, 64> knight_attack_masks;
	extern const std::array<bitboard, 64> bishop_attack_masks;
	extern const std::array<bitboard, 64> rook_attack_masks;
	extern const std::array<bitboard, 64> king_attack_masks;

	extern const std::array<bitboard, 64> rook_pext_masks;
	using rook_move_masks_t = std::unique_ptr<std::array<std::array<bitboard, 4096>, 64>>;
	extern const rook_move_masks_t rook_move_masks;

	extern const std::array<bitboard, 64> bishop_pext_masks;
	using bishop_move_masks_t = std::unique_ptr<std::array<std::array<bitboard, 512>, 64>>;
	extern const bishop_move_masks_t bishop_move_masks;

	constexpr bitboard rank_8 = 0xFF;
	constexpr bitboard rank_7 = rank_8 << 8;
	constexpr bitboard rank_6 = rank_7 << 8;
	constexpr bitboard rank_5 = rank_6 << 8;
	constexpr bitboard rank_4 = rank_5 << 8;
	constexpr bitboard rank_3 = rank_4 << 8;
	constexpr bitboard rank_2 = rank_3 << 8;
	constexpr bitboard rank_1 = rank_2 << 8;

	constexpr bitboard file_a = 0x0101010101010101;
	constexpr bitboard file_b = file_a << 1;
	constexpr bitboard file_c = file_b << 1;
	constexpr bitboard file_d = file_c << 1;
	constexpr bitboard file_e = file_d << 1;
	constexpr bitboard file_f = file_e << 1;
	constexpr bitboard file_g = file_f << 1;
	constexpr bitboard file_h = file_g << 1;

	constexpr bitboard pawn_capture_lower_file = ~rank_1 & ~rank_8 & ~file_a;
	constexpr bitboard pawn_capture_higher_file = pawn_capture_lower_file >> 1;

	// Centers on a5. Shift left by ep_file. If black is moving, also shift left by 8.
	constexpr bitboard ep_capture_mask = 0b10'10000000'00000000'00000000;

	class bitboards
	{
	public:
		bitboard white, black;
		bitboard pawns, knights, bishops, rooks, queens, kings;

		bool operator==(const bitboards&) const = default;

		const bitboard occupied() const { return white | black; }
		const bitboard empty() const { return ~occupied(); }

		template<color_t color>
		const bitboard& get() const
		{
			return ((color == chess::white) ? white : black);
		}

		template<color_t color, piece_t piece>
		const bitboard get() const
		{
			return get<color>() &
				((piece == pawn) ? pawns :
				 (piece == knight) ? knights :
				 (piece == bishop) ? bishops :
				 (piece == rook) ? rooks :
				 (piece == queen) ? queens : kings);
		}

		void print() const;
	};

	template<piece_t piece>
	__forceinline bitboard get_bitboard_for(const position& position)
	{
		static_assert(sizeof(chess::position) == 64);
		static_assert(alignof(chess::position) == 64);

		// load the 64 bytes of the board into two ymm registers
		uint256_t ymm0 = _mm256_loadu_si256((uint256_t*)(position._position.data() + 0));
		uint256_t ymm1 = _mm256_loadu_si256((uint256_t*)(position._position.data() + 32));

		// broadcast the target piece to all positions of a register
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

	__forceinline bitboards get_bitboards(const position& position)
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

		// load the 64 bytes of the board into two ymm registers
		const uint256_t position_lo = _mm256_loadu_si256((uint256_t*)(position._position.data() + 0));
		const uint256_t position_hi = _mm256_loadu_si256((uint256_t*)(position._position.data() + 32));

		// for white: replace white pieces with a ones-mask, then extract that mask
		const uint256_t white_piece_mask = _mm256_broadcastsi128_si256(_mm_loadu_si128((uint128_t*)&white_piece_lookup));
		const uint64_t white_hi = uint32_t(_mm256_movemask_epi8(_mm256_shuffle_epi8(white_piece_mask, position_hi))); // high half first
		const uint64_t white_lo = uint32_t(_mm256_movemask_epi8(_mm256_shuffle_epi8(white_piece_mask, position_lo)));

		// for black: shift color bits to the highest bit in their byte, then extract those bits into a mask
		const uint64_t black_hi = uint32_t(_mm256_movemask_epi8(_mm256_slli_epi64(position_hi, 7))); // high half first
		const uint64_t black_lo = uint32_t(_mm256_movemask_epi8(_mm256_slli_epi64(position_lo, 7)));

		const bitboard white_bb = (white_hi << 32) | white_lo;
		const bitboard black_bb = (black_hi << 32) | black_lo;

		bitboards bitboards;

		bitboards.white = white_bb;
		bitboards.black = black_bb;

		bitboards.pawns = get_bitboard_for<white | pawn>(position) | get_bitboard_for<black | pawn>(position);
		bitboards.knights = get_bitboard_for<white | knight>(position) | get_bitboard_for<black | knight>(position);
		bitboards.bishops = get_bitboard_for<white | bishop>(position) | get_bitboard_for<black | bishop>(position);
		bitboards.rooks = get_bitboard_for<white | rook>(position) | get_bitboard_for<black | rook>(position);
		bitboards.queens = get_bitboard_for<white | queen>(position) | get_bitboard_for<black | queen>(position);
		bitboards.kings = get_bitboard_for<white | king>(position) | get_bitboard_for<black | king>(position);

		return bitboards;
	}

	inline size_t get_next_bit_index(const bitboard bitboard)
	{
		return ::util::tzcnt(bitboard);
	}

	inline bitboard get_next_bit(const bitboard bitboard)
	{
		return ::util::blsi(bitboard);
	}

	[[nodiscard]] inline bitboard clear_next_bit(const bitboard bitboard)
	{
		return ::util::blsr(bitboard);
	}

	template<piece_t piece>
	bitboard get_slider_moves(const bitboards& bitboards, const size_t idx)
	{
		static_assert(piece == bishop || piece == rook || piece == queen);

		bitboard bishop_pext_mask;
		bitboard rook_pext_mask;
		if constexpr (piece == bishop || piece == queen)
			bishop_pext_mask = bishop_pext_masks[idx];
		if constexpr (piece == rook || piece == queen)
			rook_pext_mask = rook_pext_masks[idx];

		const bitboard occupied = bitboards.occupied();

		size_t bishop_movemask_idx;
		size_t rook_movemask_idx;
		if constexpr (piece == bishop || piece == queen)
			bishop_movemask_idx = pext(occupied, bishop_pext_mask);
		if constexpr (piece == rook || piece == queen)
			rook_movemask_idx = pext(occupied, rook_pext_mask);

		bitboard moves;
		if constexpr (piece == bishop || piece == queen)
			moves |= (*bishop_move_masks)[idx][bishop_movemask_idx];
		if constexpr (piece == rook || piece == queen)
			moves |= (*rook_move_masks)[idx][rook_movemask_idx];

		return moves;
	}
	template<piece_t piece>
	bitboard get_slider_moves(const bitboards& bitboards, const bitboard square)
	{
		return get_slider_moves<piece>(bitboards, get_next_bit_index(square));
	}

	template<color_t king_color>
	force_inline_toggle bool is_attacked_by_sliding_piece(const bitboards& bitboards,
														  const bitboard king_position)
	{
		const size_t king_idx = get_next_bit_index(king_position);

		const bitboard rook_pext_mask = rook_pext_masks[king_idx];
		const bitboard bishop_pext_mask = bishop_pext_masks[king_idx];
		const bitboard occupied = bitboards.white | bitboards.black;
		const size_t rook_movemask_idx = pext(occupied, rook_pext_mask);
		const size_t bishop_movemask_idx = pext(occupied, bishop_pext_mask);

		const bitboard rook_moves = (*rook_move_masks)[king_idx][rook_movemask_idx];
		const bitboard bishop_moves = (*bishop_move_masks)[king_idx][bishop_movemask_idx];

		constexpr color_t opp_color = other_color(king_color);
		const bitboard opp_rooks_and_queens = (bitboards.rooks | bitboards.queens) & bitboards.get<opp_color>();
		const bitboard opp_bishops_and_queens = (bitboards.bishops | bitboards.queens) & bitboards.get<opp_color>();

		const bitboard checkers = (opp_rooks_and_queens & rook_moves) | (opp_bishops_and_queens & bishop_moves);

		return checkers != 0;
	}
}
