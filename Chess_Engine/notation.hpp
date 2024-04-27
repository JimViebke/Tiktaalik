#pragma once

#include "node.hpp"

namespace chess
{
	inline file char_to_file(const char c) { return c - 'a'; }
	inline rank char_to_rank(const char c) { return 8 - (c - '0'); }

	void move_to_notation(std::stringstream& ss,
						  const Node& parent_node,
						  const Node& result_node);

	std::string move_to_notation_str(const Node& parent_node,
									 const Node& result_node);
}
