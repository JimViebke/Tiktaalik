#pragma once

#include <sstream>

#include "types.hpp"

namespace chess
{
	inline file char_to_file(const char c) { return c - 'a'; }
	inline rank char_to_rank(const char c) { return 8 - (c - '0'); }

	void move_to_notation(std::stringstream& ss,
						  const size_t parent_idx,
						  const size_t child_idx,
						  const color_t child_color_to_move);
}
