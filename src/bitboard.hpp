#pragma once

#include <array>

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

	using movemasks = std::array<bitboard, 64>;

	extern const movemasks knight_movemasks;
	extern const movemasks bishop_movemasks;
	extern const movemasks rook_movemasks;
	extern const movemasks king_movemasks;

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
		bitboard white;
		bitboard black;
		bitboard empty;
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

		bitboards bitboards;
		bitboards.white = (white_hi << 32) | white_lo;
		bitboards.black = (black_hi << 32) | black_lo;
		bitboards.empty = get_bitboard_for<empty>(position);

		return bitboards;
	}

	inline size_t get_next_bit(const bitboard bitboard)
	{
		return ::util::tzcnt(bitboard);
	}

	[[nodiscard]] inline bitboard clear_next_bit(const bitboard bitboard)
	{
		return ::util::blsr(bitboard);
	}

	// The positions of the opponent's queens and bishops, and queens and rooks.
	extern std::array<bitboard, 2> qb_and_qr_bitboards;
	// All of the squares that the opponent's queens, bishops, and rooks could move to, if each
	// were on an empty board. Used to cheaply determine if the king can possibly be in check
	// from a sliding piece.
	extern bitboard qbr_attack_masks;
	// Contains a bit representing the position of the piece that was captured, if any. This is used 
	// to remove any captured piece from qb_and_qr_bitboards before performing sliding piece checks.
	extern bitboard captured_piece;

	template<color_t color>
	force_inline_toggle void set_up_qb_and_qr_bitboards_for(const position& position)
	{
		// broadcast the target piece to all positions of a register
		const uint256_t piece_mask_q = _mm256_set1_epi8(color | queen);
		const uint256_t piece_mask_r = _mm256_set1_epi8(color | rook);
		const uint256_t piece_mask_b = _mm256_set1_epi8(color | bishop);

		// load the 64 bytes of the board into two ymm registers
		const uint256_t ymm0 = _mm256_loadu_si256((uint256_t*)(position._position.data() + 0));
		const uint256_t ymm1 = _mm256_loadu_si256((uint256_t*)(position._position.data() + 32));

		// find the matching bytes
		const uint256_t q_mask_hi = _mm256_cmpeq_epi8(ymm1, piece_mask_q); // high half first
		const uint256_t q_mask_lo = _mm256_cmpeq_epi8(ymm0, piece_mask_q);

		// combine hi and lo halves of the masks, and extract
		const uint256_t r_mask_hi = _mm256_cmpeq_epi8(ymm1, piece_mask_r); // high half first
		const uint64_t qr_mask_hi = uint32_t(_mm256_movemask_epi8(_mm256_or_si256(q_mask_hi, r_mask_hi)));
		const uint256_t r_mask_lo = _mm256_cmpeq_epi8(ymm0, piece_mask_r);
		const uint64_t qr_mask_lo = uint32_t(_mm256_movemask_epi8(_mm256_or_si256(q_mask_lo, r_mask_lo)));
		const uint256_t b_mask_hi = _mm256_cmpeq_epi8(ymm1, piece_mask_b); // high half first
		const uint64_t qb_mask_hi = uint32_t(_mm256_movemask_epi8(_mm256_or_si256(q_mask_hi, b_mask_hi)));
		const uint256_t b_mask_lo = _mm256_cmpeq_epi8(ymm0, piece_mask_b);
		const uint64_t qb_mask_lo = uint32_t(_mm256_movemask_epi8(_mm256_or_si256(q_mask_lo, b_mask_lo)));

		bitboard queens_and_rooks_bb = (qr_mask_hi << 32) | qr_mask_lo; // queens | rooks;
		bitboard queens_and_bishops_bb = (qb_mask_hi << 32) | qb_mask_lo; // queens | bishops;

		qb_and_qr_bitboards[0] = queens_and_bishops_bb;
		qb_and_qr_bitboards[1] = queens_and_rooks_bb;
		captured_piece = 0;

		bitboard attack_mask = 0;
		while (queens_and_bishops_bb)
		{
			const size_t index = get_next_bit(queens_and_bishops_bb);
			attack_mask |= bishop_movemasks[index];
			queens_and_bishops_bb = clear_next_bit(queens_and_bishops_bb);
		}

		while (queens_and_rooks_bb)
		{
			const size_t index = get_next_bit(queens_and_rooks_bb);
			attack_mask |= rook_movemasks[index];
			queens_and_rooks_bb = clear_next_bit(queens_and_rooks_bb);
		}

		qbr_attack_masks = attack_mask;
	}

	template<color_t king_color>
	force_inline_toggle bool is_attacked_by_sliding_piece(const position& position,
														  const bitboard king_position)
	{
		if constexpr (config::verify_cached_sliding_piece_bitboards)
		{
			// Generate the qb and qr bitboards from scratch, and see if they ever mismatch the cached version

			constexpr color_t opp_color = other_color(king_color);

			const bitboard queens = get_bitboard_for<opp_color | queen>(position);
			const bitboard bishops = get_bitboard_for<opp_color | bishop>(position);
			const bitboard rooks = get_bitboard_for<opp_color | rook>(position);

			const bitboard queens_and_bishops = queens | bishops;
			const bitboard queens_and_rooks = queens | rooks;
			const bool qb_mismatch = (queens_and_bishops != (qb_and_qr_bitboards[0] & ~captured_piece.value()));
			const bool qr_mismatch = (queens_and_rooks != (qb_and_qr_bitboards[1] & ~captured_piece.value()));

			if (qb_mismatch || qr_mismatch)
			{
				if (qb_mismatch)
				{
					std::cout << "queen/bishop bitboard mismatch\n";
					std::cout << "cached:\n";
					print_bitboard(qb_and_qr_bitboards[0]);
					std::cout << "generated:\n";
					print_bitboard(queens_and_bishops);
				}

				if (qr_mismatch)
				{
					std::cout << "queen/rook bitboard mismatch\n";
					std::cout << "cached:\n";
					print_bitboard(qb_and_qr_bitboards[1]);
					std::cout << "generated:\n";
					print_bitboard(queens_and_rooks);
				}

				std::cout << "cached captured_piece:\n";
				print_bitboard(captured_piece);
				position.print();
				std::cin.ignore();
			}
		}

		/*
		 0  1  2  3  4  5  6  7
		 8 ...
		16 ...
		24 ...
		32 ...
		40 ...
		48 ...
		56 57 58 59 60 61 62 63
		*/

		/*
		NW   N  NE
		 W       E
		SW   S  SE

		-9  -8  -7
		-1       1
		 7   8   9
		*/

		const uint256_t ymm_captured_piece = _mm256_set1_epi64x(captured_piece);
		captured_piece = 0; // clear so we don't read this again.
		// if no sliding piece could capture the king on an otherwise empty board,
		// the king can't be in check
		if ((king_position & qbr_attack_masks) == 0) return false;

		// broadcast empty bitboard to all four positions
		const uint256_t empty_squares = _mm256_set1_epi64x(get_bitboard_for<empty>(position));

		constexpr uint64_t move_se = 0xFE'FE'FE'FE'FE'FE'FE'00;
		constexpr uint64_t move__s = 0xFF'FF'FF'FF'FF'FF'FF'00;
		constexpr uint64_t move_sw = 0x7F'7F'7F'7F'7F'7F'7F'00;
		constexpr uint64_t move__e = 0xFE'FE'FE'FE'FE'FE'FE'FE;
		constexpr uint64_t move__w = 0x7F'7F'7F'7F'7F'7F'7F'7F;
		constexpr uint64_t move_ne = 0x00'FE'FE'FE'FE'FE'FE'FE;
		constexpr uint64_t move__n = 0x00'FF'FF'FF'FF'FF'FF'FF;
		constexpr uint64_t move_nw = 0x00'7F'7F'7F'7F'7F'7F'7F;
		constexpr static std::array<uint64_t, 4> static_right_shift_masks = { move_nw, move__n, move_ne, move__w };
		constexpr static std::array<uint64_t, 4> static_left_shift_masks = { move_se, move__s, move_sw, move__e };
		uint256_t right_shift_masks = _mm256_loadu_si256((uint256_t*)&static_right_shift_masks);
		uint256_t left_shift_masks = _mm256_loadu_si256((uint256_t*)&static_left_shift_masks);

		// load 2x slider bitmasks created by generate_child_boards() into 4x positions
		uint256_t sliders = _mm256_broadcastsi128_si256(_mm_loadu_si128((uint128_t*)qb_and_qr_bitboards.data()));

		// Remove any slider that was just captured. Equivalent to:
		// sliders = _mm256_andnot_si256(ymm_captured_piece, sliders);
		asm(R"(VPANDN	%[sliders], %[ymm_captured_piece], %[sliders])"
			: [sliders] "+&v" (sliders)
			: [ymm_captured_piece] "v" (ymm_captured_piece));

		constexpr static std::array<uint8_t, 4> shift_amounts = { 9, 8, 7, 1 };
		const uint256_t shifts = _mm256_cvtepi8_epi64(_mm_loadu_si32(shift_amounts.data()));

		// select all empty bits within the allowable regions
		uint256_t right_shift_and_empty = _mm256_and_si256(right_shift_masks, empty_squares);
		uint256_t left_shift_and_empty = _mm256_and_si256(left_shift_masks, empty_squares);

		// shift left and right by [7, 8, 9, 1]
		uint256_t right_temp = _mm256_srlv_epi64(sliders, shifts);
		uint256_t left_temp = _mm256_sllv_epi64(sliders, shifts);
		// select only the pieces that can correctly end up here
		right_temp = _mm256_and_si256(right_temp, right_shift_and_empty);
		left_temp = _mm256_and_si256(left_temp, left_shift_and_empty);
		// accumulate movements
		uint256_t right_sliders = _mm256_or_si256(sliders, right_temp);
		uint256_t left_sliders = _mm256_or_si256(sliders, left_temp);

		// empty &= (empty >> shift);
		right_shift_and_empty = _mm256_and_si256(right_shift_and_empty, _mm256_srlv_epi64(right_shift_and_empty, shifts));
		left_shift_and_empty = _mm256_and_si256(left_shift_and_empty, _mm256_sllv_epi64(left_shift_and_empty, shifts));

		const uint256_t shifts_2 = _mm256_add_epi64(shifts, shifts);

		// shift left and right by [7, 8, 9, 1]*2
		right_temp = _mm256_srlv_epi64(right_sliders, shifts_2);
		left_temp = _mm256_sllv_epi64(left_sliders, shifts_2);
		// select only the pieces that can correctly end up here
		right_temp = _mm256_and_si256(right_temp, right_shift_and_empty);
		left_temp = _mm256_and_si256(left_temp, left_shift_and_empty);
		// accumulate movements
		right_sliders = _mm256_or_si256(right_sliders, right_temp);
		left_sliders = _mm256_or_si256(left_sliders, left_temp);

		// empty &= (empty >> shift_2);
		right_shift_and_empty = _mm256_and_si256(right_shift_and_empty, _mm256_srlv_epi64(right_shift_and_empty, shifts_2));
		left_shift_and_empty = _mm256_and_si256(left_shift_and_empty, _mm256_sllv_epi64(left_shift_and_empty, shifts_2));

		const uint256_t shifts_3 = _mm256_add_epi64(shifts_2, shifts_2);

		// shift left and right by [7, 8, 9, 1]*4
		right_temp = _mm256_srlv_epi64(right_sliders, shifts_3);
		left_temp = _mm256_sllv_epi64(left_sliders, shifts_3);
		// select only the pieces that can correctly end up here
		right_temp = _mm256_and_si256(right_temp, right_shift_and_empty);
		left_temp = _mm256_and_si256(left_temp, left_shift_and_empty);
		// accumulate movements
		right_sliders = _mm256_or_si256(right_sliders, right_temp);
		left_sliders = _mm256_or_si256(left_sliders, left_temp);

		// (no intermediate third step, because we use the original masks)

		// 4th and final step; shift the accumulated masks by the original shift
		right_sliders = _mm256_srlv_epi64(right_sliders, shifts);
		left_sliders = _mm256_sllv_epi64(left_sliders, shifts);
		// select only the pieces that can correctly end up here
		right_sliders = _mm256_and_si256(right_sliders, right_shift_masks);
		left_sliders = _mm256_and_si256(left_sliders, left_shift_masks);
		// combine 8 sliding attack maps into 4
		const uint256_t out = _mm256_or_si256(left_sliders, right_sliders);

		// if any of the masks intersect with the king (sliders & king), the king is being checked by a slider
		return _mm256_testnzc_si256(out, _mm256_set1_epi64x(king_position));
	}

	template<piece_t piece_type, typename generate_moves_fn_t>
	force_inline_toggle void find_moves_for(size_t& out_index, const size_t parent_idx,
											const size_t king_index, const bool started_in_check,
											const tt::key key, generate_moves_fn_t generate_moves_fn)
	{
		const position& parent_position = positions[parent_idx];
		bitboard pieces = get_bitboard_for<piece_type>(parent_position);

		while (pieces)
		{
			const size_t piece_idx = get_next_bit(pieces);
			pieces = clear_next_bit(pieces);

			// XOR the key for the leaving piece once for all of its moves
			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[piece_type][piece_idx];

			generate_moves_fn(out_index, parent_idx, piece_idx / 8, piece_idx % 8, king_index, started_in_check, incremental_key);
		}
	}
}
