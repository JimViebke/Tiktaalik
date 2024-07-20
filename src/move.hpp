#pragma once

#include "board.hpp"
#include "position.hpp"

namespace chess
{
	template<color_t color_to_move, bool perft = false>
	size_t generate_child_boards(const size_t parent_idx);
}
