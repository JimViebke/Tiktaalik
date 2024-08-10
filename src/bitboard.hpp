#pragma once

#include <array>
#include <memory>

#include "config.hpp"
#include "piece.hpp"
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

		const bitboard occupied() const { return white | black; }
		const bitboard empty() const { return ~occupied(); }

		template <color_t color>
		const bitboard& get() const
		{
			return ((color == chess::white) ? white : black);
		}

		template <color_t color, piece_t piece>
		const bitboard get() const
		{
			return get<color>() & ((piece == pawn)        ? pawns
			                          : (piece == knight) ? knights
			                          : (piece == bishop) ? bishops
			                          : (piece == rook)   ? rooks
			                          : (piece == queen)  ? queens
			                                              : kings);
		}

		void print() const;
	};

	inline size_t get_next_bit_index(const bitboard bitboard) { return ::util::tzcnt(bitboard); }

	inline bitboard get_next_bit(const bitboard bitboard) { return ::util::blsi(bitboard); }

	[[nodiscard]] inline bitboard clear_next_bit(const bitboard bitboard) { return ::util::blsr(bitboard); }

	template <piece_t piece>
	force_inline_toggle bitboard get_slider_moves(const bitboards& bitboards, const size_t start_idx)
	{
		static_assert(piece == bishop || piece == rook || piece == queen);

		bitboard bishop_pext_mask;
		bitboard rook_pext_mask;
		if constexpr (piece == bishop || piece == queen)
		{
			bishop_pext_mask = bishop_pext_masks[start_idx];
		}
		if constexpr (piece == rook || piece == queen)
		{
			rook_pext_mask = rook_pext_masks[start_idx];
		}

		const bitboard occupied = bitboards.occupied();

		size_t bishop_movemask_idx;
		size_t rook_movemask_idx;
		if constexpr (piece == bishop || piece == queen)
		{
			bishop_movemask_idx = pext(occupied, bishop_pext_mask);
		}
		if constexpr (piece == rook || piece == queen)
		{
			rook_movemask_idx = pext(occupied, rook_pext_mask);
		}

		bitboard moves;
		if constexpr (piece == bishop || piece == queen)
		{
			moves |= (*bishop_move_masks)[start_idx][bishop_movemask_idx];
		}
		if constexpr (piece == rook || piece == queen)
		{
			moves |= (*rook_move_masks)[start_idx][rook_movemask_idx];
		}

		return moves;
	}
	template <piece_t piece>
	force_inline_toggle bitboard get_slider_moves(const bitboards& bitboards, const bitboard square)
	{
		return get_slider_moves<piece>(bitboards, get_next_bit_index(square));
	}

	template <color_t color>
	force_inline_toggle bitboard get_blockers(const bitboards& bitboards)
	{
		const bitboard our_pieces = bitboards.get<color>();
		const bitboard king_bitboard = our_pieces & bitboards.kings;
		const size_t king_idx = get_next_bit_index(king_bitboard);
		const bitboard moves = get_slider_moves<queen>(bitboards, king_idx);
		const bitboard blocker_squares = bishop_pext_masks[king_idx] | rook_pext_masks[king_idx];
		return our_pieces & blocker_squares & moves;
	}

	template <color_t king_color>
	force_inline_toggle bool is_attacked_by_sliding_piece(const bitboards& bitboards, const size_t king_idx)
	{
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
