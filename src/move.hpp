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

	template<color_t color_to_move, gen_moves gen_moves = gen_moves::all, bool perft = false>
	size_t generate_child_boards(const size_t parent_idx);
}
