#pragma once

#include "board.hpp"
#include "position.hpp"

namespace chess
{
	enum class gen_moves
	{
		all,
		captures
	};

	template<color_t attacker_color>
	inline_toggle bool square_is_attacked_by_pawn(const position& position, const rank rank, const file file)
	{
		if constexpr (attacker_color == white)
		{
			if ((bounds_check(rank + 1, file + 1) && position.piece_at(rank + 1, file + 1).is(attacker_color | pawn)) ||
				(bounds_check(rank + 1, file - 1) && position.piece_at(rank + 1, file - 1).is(attacker_color | pawn))) return true;
		}
		else
		{
			if ((bounds_check(rank - 1, file + 1) && position.piece_at(rank - 1, file + 1).is(attacker_color | pawn)) ||
				(bounds_check(rank - 1, file - 1) && position.piece_at(rank - 1, file - 1).is(attacker_color | pawn))) return true;
		}

		return false;
	}
	template<color_t attacker_color>
	inline_toggle bool square_is_attacked_by_knight(const position& position, const rank rank, const file file)
	{
		return knight_attacks[to_index(rank, file)](position, attacker_color | knight);
	}

	template<color_t king_color>
	inline size_t find_king_index(const position& position)
	{
		// Scan for the position of the first set bit in the mask.
		// Assume that the board will always have a king of a given color.
		return get_next_bit(get_bitboard_for<king_color | king>(position));
	}

	template<color_t color_to_move, gen_moves gen_moves = gen_moves::all, bool perft = false>
	size_t generate_child_boards(const size_t parent_idx);
}
