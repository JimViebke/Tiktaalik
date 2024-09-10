#pragma once

#include "bitboard.hpp"
#include "board.hpp"

namespace chess
{
	enum class gen_moves
	{
		all,
		captures,
		noncaptures
	};

	template <color color_to_move, gen_moves gen_moves = gen_moves::all, bool quiescing = false, bool perft = false>
	size_t generate_child_boards(const size_t parent_idx);
}
