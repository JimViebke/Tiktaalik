#pragma once

#include "board.hpp"
#include "position.hpp"

namespace chess
{
	template<color_t child_color>
	void make_move(position& child, const position& parent, const board& child_board);

	bool is_king_in_check(const position& position, const color_t king_color);

	template<color_t color_to_move, bool perft = false>
	size_t generate_child_boards(const size_t parent_idx);
}
