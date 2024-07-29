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
