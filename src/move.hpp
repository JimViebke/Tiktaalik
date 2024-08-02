#pragma once

#include "bitboard.hpp"
#include "board.hpp"
#include "position.hpp"

namespace chess
{
	template<color_t attacker_color>
	[[clang::always_inline]] static bool square_is_attacked_by_pawn(
		const bitboards& bitboards, const size_t index)
	{
		const bitboard opp_pawns = bitboards.get<attacker_color>() & bitboards.pawns;
		const bitboard index_bb = 1ull << index;

		const bitboard checkers_to_lower_file = opp_pawns & pawn_capture_lower_file
			& ((attacker_color == white) ? index_bb << 9 : index_bb >> 7);
		const bitboard checkers_to_higher_file = opp_pawns & pawn_capture_higher_file
			& ((attacker_color == white) ? index_bb << 7 : index_bb >> 9);

		return checkers_to_lower_file | checkers_to_higher_file;
	}

	template<color_t attacker_color>
	[[clang::always_inline]] static bool square_is_attacked_by_knight(
		const bitboards& bitboards, const size_t index)
	{
		const bitboard opp_knights = bitboards.get<attacker_color>() & bitboards.knights;
		return opp_knights & knight_attack_masks[index];
	}

	template<color_t attacker_color>
	[[clang::always_inline]] static bool square_is_attacked_by_king(
		const bitboards& bitboards, const size_t index)
	{
		const bitboard opp_king = bitboards.get<attacker_color>() & bitboards.kings;
		return opp_king & king_attack_masks[index];
	}

	// If the opponent isn't checking with a pawn, skip pawn checks.
	// If the opponent isn't checking with a knight, skip knight checks.
	enum class check_type
	{
		do_pawn_checks, // implies skipping knight and king checks
		do_knight_checks, // implies skipping pawn and king checks
		skip_pawn_and_knight_checks, // nominal case (only do sliding piece checks)

		do_all // do all checks
	};

	template<color_t king_color, check_type check_type>
	force_inline_toggle bool is_king_in_check(const bitboards& bitboards, const rank rank, const file file)
	{
		constexpr color_t opp_color = other_color(king_color);
		const size_t king_index = to_index(rank, file);

		if (is_attacked_by_sliding_piece<king_color>(bitboards, 1ull << king_index)) return true;

		if constexpr (check_type == check_type::do_all)
		{
			if (square_is_attacked_by_king<opp_color>(bitboards, king_index)) return true;
		}

		if constexpr (check_type == check_type::do_knight_checks ||
					  check_type == check_type::do_all)
		{
			if (square_is_attacked_by_knight<opp_color>(bitboards, king_index)) return true;
		}

		if constexpr (check_type == check_type::do_pawn_checks ||
					  check_type == check_type::do_all)
		{
			if (square_is_attacked_by_pawn<opp_color>(bitboards, king_index)) return true;
		}

		return false;
	}

	enum class gen_moves
	{
		all,
		captures,
		noncaptures
	};

	template<color_t color_to_move, gen_moves gen_moves = gen_moves::all, bool perft = false>
	size_t generate_child_boards(const size_t parent_idx);
}
